/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_i2c.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* I2C driver for the CH32H417:
 *   - I2C1 master on /dev/i2c0 (PB6 SCL / PB7 SDA, AF4)
 *   - I2C2 master on /dev/i2c1 (PC0 SCL / PC1 SDA, AF9)
 *   - I2C2 hardware-slave responder at 0x50 (CONFIG_CH32H41X_I2C2_SLAVE)
 *
 * Sequences follow the WCH EVT reference (ch32h417_i2c.c, I2C_7bit_Mode
 * demo) and RM chapter 21.  Cross-checked items that differ from generic
 * STM32F1 habits:
 *  - The peripheral clock is HCLK (100MHz); CTLR2.FREQ caps at 60.
 *  - OADDR1: RM says bit15 ADDMODE (0=7bit) and bit14 reserved; the EVT
 *    library additionally sets bit14 (I2C_AcknowledgedAddress_7bit =
 *    0x4000).  We follow the EVT constant so the responder behaves like
 *    the vendor example.
 *  - STAR1/STAR2 are 16-bit registers; event composition uses
 *    (STAR2 << 16) | STAR1 like EVT I2C_GetLastEvent.
 *
 * Pins (CH32H417DS0 pin table), open-drain AF (RM table 9-7):
 *   I2C1: PB6 = SCL (AF4), PB7 = SDA (AF4)
 *   I2C2: PC0 = SCL (AF9), PC1 = SDA (AF9)
 *
 * Dual-controller self test: I2C2 also runs as a hardware SLAVE at
 * address 0x50 (CONFIG_CH32H41X_I2C2_SLAVE).  Its ISR answers the
 * master: writes land in a small RX buffer, reads return the same
 * buffer (set -> get round trip).  With jumpers PB6-PC0 and PB7-PC1,
 * `i2ctool -b 0 detect` on the I2C1 master discovers the slave.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <nuttx/arch.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/irq.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_gpio.h"
#include "hardware/ch32h41x_i2c.h"

#include "ch32h41x_internal.h"

#ifdef CONFIG_CH32H41X_I2C

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define I2C1_BASE CH32H41X_I2C1_BASE
#define I2C2_BASE CH32H41X_I2C2_BASE

#define I2C_SPINLIMIT 200000ul

/* Slave responder (I2C2) */

#define I2C_SLAVE_ADDR     0x50
#define I2C_SLAVE_BUFSIZE  16ul

/* I2C2 interrupt lines (NuttX ids) */

#define I2C2EV_IRQ  (CH32H41X_IRQ_FIRST + 52)
#define I2C2ER_IRQ  (CH32H41X_IRQ_FIRST + 53)

/* STAR2 flags composed EVT-style: (STAR2 << 16) | STAR1 */

#define I2C_EV_MSL   (1ul << 16)
#define I2C_EV_BUSY  (1ul << 17)
#define I2C_EV_TRA   (1ul << 18)

#define I2C_ERRMASK  (CH32H41X_I2C_SR1_AF | CH32H41X_I2C_SR1_ARLO | \
                      CH32H41X_I2C_SR1_BERR | CH32H41X_I2C_SR1_OVR)

/* GPIO AF numbers (CH32H417DS0 pin table) */

#define I2C1_AF  4ul
#define I2C2_AF  9ul

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* One per controller.  The NuttX master handle is the embedded
 * i2c_master_s (vtable container). */

struct ch32h41x_i2cm_s
{
  struct i2c_master_s master;     /* NuttX handle (first member) */
  uintptr_t base;                 /* controller register base */
  uint32_t frequency;             /* current CCR bit rate */
};

#ifdef CONFIG_CH32H41X_I2C2_SLAVE
struct ch32h41x_i2c_slave_s
{
  uint8_t rxbuf[I2C_SLAVE_BUFSIZE];
  volatile uint8_t rxidx;         /* next RX slot */
  volatile uint8_t txidx;         /* next TX byte */
  volatile uint8_t nrx;           /* bytes received in the last write */
  volatile uint8_t matched;
};
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ch32h41x_i2cm_s g_i2cm[2];

#ifdef CONFIG_CH32H41X_I2C2_SLAVE
static struct ch32h41x_i2c_slave_s g_i2cslave;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_i2c_wait_star1
 *
 * Description:
 *   Poll STAR1 until any of the given flags are set (bounded).  Error
 *   flags abort the wait.  Returns the STAR1 value, or a negated errno.
 *
 ****************************************************************************/

static int ch32h41x_i2c_wait_star1(uintptr_t base, uint16_t flags)
{
  uint32_t timeout = I2C_SPINLIMIT;
  uint16_t star1;

  for (; ; )
    {
      star1 = getreg16(CH32H41X_I2C_STAR1(base));

      if ((star1 & flags) != 0)
        {
          return star1;
        }

      if ((star1 & I2C_ERRMASK) != 0)
        {
          return -EIO;
        }

      if (--timeout == 0)
        {
          return -ETIMEDOUT;
        }
    }
}

/****************************************************************************
 * Name: ch32h41x_i2c_clear_addr
 *
 * Description:
 *   Clear the ADDR flag: read STAR1 then read STAR2 (RM/EVT two-read
 *   sequence).
 *
 ****************************************************************************/

static void ch32h41x_i2c_clear_addr(uintptr_t base)
{
  (void)getreg16(CH32H41X_I2C_STAR1(base));
  (void)getreg16(CH32H41X_I2C_STAR2(base));
}

/****************************************************************************
 * Name: ch32h41x_i2c_ccr
 *
 * Description:
 *   Recompute CKCFGR/RTR for the requested bit rate (EVT I2C_Init
 *   algorithm: FREQ field capped at 60, standard mode CCR =
 *   pclk/(2*speed), fast mode with 16/9 duty = pclk/(speed*25)).
 *   PE must be off while the registers change; it is re-enabled.
 *
 ****************************************************************************/

static void ch32h41x_i2c_ccr(uintptr_t base, uint32_t frequency)
{
  uint32_t pclk = ch32h41x_hclk_frequency();
  uint32_t freqrange = pclk / 1000000ul;
  uint32_t ccr;

  if (freqrange > 60)
    {
      freqrange = 60;
    }

  modifyreg32(CH32H41X_I2C_CTLR2(base),
              CH32H41X_I2C_CR2_FREQ_MASK, freqrange);

  modifyreg32(CH32H41X_I2C_CTLR1(base), CH32H41X_I2C_CR1_PE, 0);

  if (frequency <= 100000ul)
    {
      /* Standard mode */

      ccr = pclk / (frequency * 2ul);
      if (ccr < 4)
        {
          ccr = 4;
        }

      modifyreg32(CH32H41X_I2C_CKCFGR(base),
                  CH32H41X_I2C_CK_CCR_FAST_MASK | CH32H41X_I2C_CK_FS, 0);
      putreg16((uint16_t)(freqrange + 1), CH32H41X_I2C_RTR(base));
    }
  else
    {
      /* Fast mode, 16/9 duty (EVT I2C_DutyCycle_16_9 path) */

      ccr = pclk / (frequency * 25ul);
      if ((ccr & 0xffful) == 0)
        {
          ccr |= 1;
        }

      modifyreg32(CH32H41X_I2C_CKCFGR(base),
                  CH32H41X_I2C_CK_CCR_FAST_MASK | CH32H41X_I2C_CK_FS,
                  (ccr & 0xffful) | CH32H41X_I2C_CK_DUTY |
                  CH32H41X_I2C_CK_FS);
      putreg16((uint16_t)((freqrange * 300ul / 1000ul) + 1),
               CH32H41X_I2C_RTR(base));
    }

  modifyreg32(CH32H41X_I2C_CTLR1(base), 0, CH32H41X_I2C_CR1_PE);
}

/****************************************************************************
 * Name: ch32h41x_i2c_transfer
 *
 * Description:
 *   Execute a sequence of messages as master (polled, bounded waits).
 *
 ****************************************************************************/

static int ch32h41x_i2c_transfer(FAR struct i2c_master_s *dev,
                                 FAR struct i2c_msg_s *msgs, int count)
{
  FAR struct ch32h41x_i2cm_s *priv =
    (FAR struct ch32h41x_i2cm_s *)dev;
  uintptr_t base = priv->base;
  int m;
  int ret = OK;

  if (msgs[0].frequency != priv->frequency)
    {
      ch32h41x_i2c_ccr(base, msgs[0].frequency);
      priv->frequency = msgs[0].frequency;
    }

  for (m = 0; m < count; m++)
    {
      FAR struct i2c_msg_s *msg = &msgs[m];
      bool read = (msg->flags & I2C_M_READ) != 0;
      uint32_t timeout = I2C_SPINLIMIT;
      uint16_t star1;
      int i;

      /* Wait for the bus to be free */

      while ((getreg16(CH32H41X_I2C_STAR2(base)) &
              CH32H41X_I2C_SR2_BUSY_S) != 0)
        {
          if (--timeout == 0)
            {
              return -ETIMEDOUT;
            }
        }

      /* START, wait SB; SB clears on read STAR1 + write DATAR */

      modifyreg32(CH32H41X_I2C_CTLR1(base), 0, CH32H41X_I2C_CR1_START);

      ret = ch32h41x_i2c_wait_star1(base, CH32H41X_I2C_SR1_SB);
      if (ret < 0)
        {
          goto errout;
        }

      (void)getreg16(CH32H41X_I2C_STAR1(base));

      /* Address byte; writing DATAR also clears SB */

      putreg8((uint8_t)((msg->addr << 1) | (read ? 1 : 0)),
              CH32H41X_I2C_DATAR(base));

      ret = ch32h41x_i2c_wait_star1(base, CH32H41X_I2C_SR1_ADDR);
      if (ret < 0)
        {
          if (ret == -EIO)
            {
              /* NACK from the slave: clear AF, release the bus */

              modifyreg32(CH32H41X_I2C_STAR1(base),
                          CH32H41X_I2C_SR1_AF, 0);
              modifyreg32(CH32H41X_I2C_CTLR1(base), 0,
                          CH32H41X_I2C_CR1_STOP);
            }

          goto errout;
        }

      if (!read)
        {
          /* Master write: ADDR clears on the STAR1/STAR2 read pair */

          ch32h41x_i2c_clear_addr(base);

          for (i = 0; i < msg->length; i++)
            {
              putreg8(msg->buffer[i], CH32H41X_I2C_DATAR(base));

              if (i < msg->length - 1)
                {
                  ret = ch32h41x_i2c_wait_star1(base,
                                                CH32H41X_I2C_SR1_TXE);
                }
              else
                {
                  ret = ch32h41x_i2c_wait_star1(base,
                                                CH32H41X_I2C_SR1_BTF);
                }

              if (ret < 0)
                {
                  goto errout;
                }
            }

          modifyreg32(CH32H41X_I2C_CTLR1(base), 0,
                      CH32H41X_I2C_CR1_STOP);
        }
      else
        {
          /* Master read.  ACK planning: for a single byte, disable ACK
           * before clearing ADDR; for more, keep ACK until the (n-1)th
           * byte.  STOP fires after the final byte read. */

          if (msg->length == 1)
            {
              modifyreg32(CH32H41X_I2C_CTLR1(base),
                          CH32H41X_I2C_CR1_ACK, 0);
              ch32h41x_i2c_clear_addr(base);
              modifyreg32(CH32H41X_I2C_CTLR1(base), 0,
                          CH32H41X_I2C_CR1_STOP);

              ret = ch32h41x_i2c_wait_star1(base,
                                            CH32H41X_I2C_SR1_RXNE);
              if (ret < 0)
                {
                  goto errout;
                }

              msg->buffer[0] = getreg8(CH32H41X_I2C_DATAR(base));
            }
          else
            {
              ch32h41x_i2c_clear_addr(base);

              for (i = 0; i < msg->length; i++)
                {
                  ret = ch32h41x_i2c_wait_star1(base,
                                                CH32H41X_I2C_SR1_RXNE);
                  if (ret < 0)
                    {
                      goto errout;
                    }

                  if (i == msg->length - 1)
                    {
                      modifyreg32(CH32H41X_I2C_CTLR1(base),
                                  CH32H41X_I2C_CR1_ACK, 0);
                      modifyreg32(CH32H41X_I2C_CTLR1(base), 0,
                                  CH32H41X_I2C_CR1_STOP);
                    }

                  msg->buffer[i] = getreg8(CH32H41X_I2C_DATAR(base));
                }

              modifyreg32(CH32H41X_I2C_CTLR1(base), 0,
                          CH32H41X_I2C_CR1_ACK);
            }
        }
    }

  return OK;

errout:
  /* Release the bus on any failure */

  modifyreg32(CH32H41X_I2C_STAR1(base), I2C_ERRMASK, 0);
  modifyreg32(CH32H41X_I2C_CTLR1(base), 0, CH32H41X_I2C_CR1_STOP);
  return ret;
}

/****************************************************************************
 * Name: i2c ops
 ****************************************************************************/

static int ch32h41x_i2c_lock(FAR struct i2c_master_s *dev, bool lock)
{
  /* Transfers are polled and serialized by the character driver */

  return OK;
}

#ifdef CONFIG_I2C_RESET
static int ch32h41x_i2c_reset(FAR struct i2c_master_s *dev)
{
  /* Clock-stretch recovery is not implemented; callers retry */

  return OK;
}
#endif

#ifdef CONFIG_CH32H41X_I2C2_SLAVE
/****************************************************************************
 * Name: ch32h41x_i2c2_slave_isr
 *
 * Description:
 *   I2C2 slave responder: ADDR match latches the direction, RXNE stores
 *   received bytes, TXE returns the mirrored buffer, STOPF ends the
 *   transfer.  Errors are cleared.
 *
 ****************************************************************************/

static int ch32h41x_i2c2_slave_isr(int irq, FAR void *context,
                                   FAR void *arg)
{
  uintptr_t base = I2C2_BASE;
  uint16_t star1 = getreg16(CH32H41X_I2C_STAR1(base));
  uint16_t star2 = getreg16(CH32H41X_I2C_STAR2(base));
  bool tra = (star2 & CH32H41X_I2C_SR2_TRA_S) != 0;

  if ((star1 & CH32H41X_I2C_SR1_ADDR) != 0)
    {
      /* ADDR clears on read STAR1 + read STAR2 (both read above) */

      g_i2cslave.matched = true;
      g_i2cslave.rxidx = 0;
      g_i2cslave.txidx = 0;
    }

  if ((star1 & CH32H41X_I2C_SR1_RXNE) != 0 && !tra)
    {
      uint8_t b = getreg8(CH32H41X_I2C_DATAR(base));

      if (g_i2cslave.rxidx < I2C_SLAVE_BUFSIZE)
        {
          g_i2cslave.rxbuf[g_i2cslave.rxidx] = b;
          g_i2cslave.rxidx++;
        }

      if (g_i2cslave.nrx < g_i2cslave.rxidx)
        {
          g_i2cslave.nrx = g_i2cslave.rxidx;
        }
    }

  if ((star1 & CH32H41X_I2C_SR1_TXE) != 0 && tra)
    {
      /* Transmit: mirror the last received buffer back to the master */

      uint8_t b = (g_i2cslave.txidx < g_i2cslave.nrx)
        ? g_i2cslave.rxbuf[g_i2cslave.txidx] : 0xff;

      putreg8(b, CH32H41X_I2C_DATAR(base));
      if (g_i2cslave.txidx < I2C_SLAVE_BUFSIZE)
        {
          g_i2cslave.txidx++;
        }
    }

  if ((star1 & CH32H41X_I2C_SR1_STOPF) != 0)
    {
      /* STOPF clears on read STAR1 + write CTLR1 */

      (void)getreg16(CH32H41X_I2C_STAR1(base));
      modifyreg32(CH32H41X_I2C_CTLR1(base), 0, CH32H41X_I2C_CR1_PE);
      g_i2cslave.matched = false;
    }

  /* Clear the error bits this pass handled (write 0 to the bit) */

  modifyreg32(CH32H41X_I2C_STAR1(base),
              star1 & I2C_ERRMASK, 0);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_i2c2_slave_init
 *
 * Description:
 *   Configure I2C2 in hardware-slave mode at 0x50 and enable the
 *   event/error interrupts.  Called once from bring-up.
 *
 ****************************************************************************/

static void ch32h41x_i2c2_slave_init(void)
{
  uintptr_t base = I2C2_BASE;

  memset(&g_i2cslave, 0, sizeof(g_i2cslave));

  modifyreg32(CH32H41X_RCC_HB1PCENR, 0, CH32H41X_RCC_HB1_I2C2);
  ch32h41x_i2c_ccr(base, 100000ul);

  putreg16((uint16_t)(I2C_SLAVE_ADDR << 1) |
           CH32H41X_I2C_OA1_ADDCONF, CH32H41X_I2C_OADDR1(base));

  modifyreg32(CH32H41X_I2C_CTLR1(base), 0,
              CH32H41X_I2C_CR1_ACK | CH32H41X_I2C_CR1_PE);
  modifyreg32(CH32H41X_I2C_CTLR2(base), 0,
              CH32H41X_I2C_CR2_ITERREN | CH32H41X_I2C_CR2_ITEVTEN |
              CH32H41X_I2C_CR2_ITBUFEN);

  irq_attach(I2C2EV_IRQ, ch32h41x_i2c2_slave_isr, NULL);
  irq_attach(I2C2ER_IRQ, ch32h41x_i2c2_slave_isr, NULL);
  up_enable_irq(I2C2EV_IRQ);
  up_enable_irq(I2C2ER_IRQ);
}
#endif /* CONFIG_CH32H41X_I2C2_SLAVE */


static const struct i2c_ops_s g_i2c1_ops =
{
  .transfer = ch32h41x_i2c_transfer,
#ifdef CONFIG_I2C_RESET
  .reset    = ch32h41x_i2c_reset,
#endif
};

static const struct i2c_ops_s g_i2c2_ops =
{
  .transfer = ch32h41x_i2c_transfer,
#ifdef CONFIG_I2C_RESET
  .reset    = ch32h41x_i2c_reset,
#endif
};

static struct ch32h41x_i2cm_s g_i2cm[2] =
{
  { { &g_i2c1_ops }, I2C1_BASE, 100000ul },
  { { &g_i2c2_ops }, I2C2_BASE, 100000ul }
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_i2c_initialize
 *
 * Description:
 *   Bring up the I2C controllers:
 *     - I2C1 master on /dev/i2c0 (PB6 SCL / PB7 SDA, AF4, 100kHz)
 *     - I2C2 master on /dev/i2c1 (PC0 SCL / PC1 SDA, AF9)
 *     - I2C2 hardware slave responder at 0x50 (CONFIG_CH32H41X_I2C2_SLAVE)
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int ch32h41x_i2c_initialize(void)
{
  static bool initialized;
  int ret;

  if (initialized)
    {
      return OK;
    }

  /* I2C1 pins: PB6 SCL / PB7 SDA, AF4, open-drain */

  modifyreg32(CH32H41X_RCC_HB1PCENR, 0, CH32H41X_RCC_HB1_I2C1);
  ch32h41x_gpio_setmode(1, 6, CH32H41X_GPIO_MODE_AF_OD_50);
  ch32h41x_gpio_setaf(1, 6, I2C1_AF);
  ch32h41x_gpio_setmode(1, 7, CH32H41X_GPIO_MODE_AF_OD_50);
  ch32h41x_gpio_setaf(1, 7, I2C1_AF);

  ret = i2c_register(&g_i2cm[0].master, 0);
  if (ret != OK)
    {
      return ret;
    }

  /* I2C2 pins: PC0 SCL / PC1 SDA, AF9, open-drain */

  modifyreg32(CH32H41X_RCC_HB1PCENR, 0, CH32H41X_RCC_HB1_I2C2);
  ch32h41x_gpio_setmode(2, 0, CH32H41X_GPIO_MODE_AF_OD_50);
  ch32h41x_gpio_setaf(2, 0, I2C2_AF);
  ch32h41x_gpio_setmode(2, 1, CH32H41X_GPIO_MODE_AF_OD_50);
  ch32h41x_gpio_setaf(2, 1, I2C2_AF);

  ret = i2c_register(&g_i2cm[1].master, 1);
  if (ret != OK)
    {
      return ret;
    }

#ifdef CONFIG_CH32H41X_I2C2_SLAVE
  ch32h41x_i2c2_slave_init();
#endif

  initialized = true;
  return OK;
}
#endif /* CONFIG_CH32H41X_I2C */
