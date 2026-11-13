/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_spi.c
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

/* SPI1 master driver for /dev/spi1 (spitool -b 1).
 *
 * The controller is the F1-style SPI (CTLR1/CTLR2/STATR/DATAR); bit
 * positions were cross-checked against the EVT ch32h417_spi.h constants
 * (SPI_Mode_Master=0x104 = MSTR|BR, NSS_Soft=0x200 = SSM, CPOL/CPHA
 * at bits 1/0, BR at [5:3], LSBFIRST=0x80, DFF=0x800).
 *
 * Pins (CH32H417DS0 pin table, all AF5):
 *   PA5 = SPI1_SCK  (AF push-pull, master)
 *   PA6 = SPI1_MISO (input floating, master)
 *   PA7 = SPI1_MOSI (AF push-pull, master)
 *   PA4 = SPI1_NSS  (unused: software NSS via SSM|SSI)
 *
 * NOTE: PA6/PA7 are also wired to the board test points used by the
 * GPIO loopback exercise; run the SPI loopback test instead of the
 * GPIO one when the SPI jumper is installed.
 *
 * Transfers are polled (TXE/RXNE flags with bounded waits); DMA is
 * available through ch32h41x_dma.c for a later high-rate pass.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>
#include <nuttx/spi/spi_transfer.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_gpio.h"
#include "hardware/ch32h41x_spi.h"

#include "ch32h41x_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SPI1_BASE  CH32H41X_SPI1_BASE

#define SPI_SPINLIMIT 100000ul

/* HCLK feeding SPI1 (set by ch32h41x_clockconfig) */

#define SPI_SRC_CLOCK ch32h41x_hclk_frequency()

/* AF selection for the SPI1 pins (CH32H417DS0 pin table) */

#define SPI1_AF       5ul

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_spi_s
{
  uint32_t frequency;             /* current bit rate */
  enum spi_mode_e mode;           /* current CPOL/CPHA mode */
  int nbits;                      /* 8 or 16 */
  sem_t lock;                     /* exclusive access */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ch32h41x_spi_s g_spi1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_spi_lock / unlock
 *
 * Description:
 *   Exclusive access.  Re-entrant in the NuttX sense (counting).
 *
 ****************************************************************************/

static int ch32h41x_spi_lock(FAR struct spi_dev_s *dev, bool lock)
{
  if (lock)
    {
      return nxsem_wait_uninterruptible(&g_spi1.lock);
    }

  nxsem_post(&g_spi1.lock);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_spi_setfrequency
 *
 * Description:
 *   Pick the largest prescaler power-of-two that does not exceed the
 *   requested bit rate: fSCK = HCLK / 2^(BR+1), BR = 0..7.
 *
 ****************************************************************************/

static uint32_t ch32h41x_spi_setfrequency(FAR struct spi_dev_s *dev,
                                          uint32_t frequency)
{
  uintptr_t base = SPI1_BASE;
  uint32_t actual;
  uint32_t reg;
  uint32_t br;

  if (frequency == g_spi1.frequency)
    {
      return g_spi1.frequency;
    }

  /* fSCK = HCLK / 2^(BR+1): find the smallest BR whose rate <= request */

  br = 0;
  while (br < 7 && (SPI_SRC_CLOCK / (2ul << (br + 1))) > frequency)
    {
      br++;
    }

  actual = SPI_SRC_CLOCK / (2ul << (br + 1));

  /* BR changes require SPE off */

  reg = getreg32(CH32H41X_SPI_CTLR1(base));
  modifyreg32(CH32H41X_SPI_CTLR1(base), CH32H41X_SPI_CR1_SPE, 0);
  reg &= ~CH32H41X_SPI_CR1_BR_MASK;
  reg |= CH32H41X_SPI_CR1_BR(br);
  putreg32(reg, CH32H41X_SPI_CTLR1(base));
  modifyreg32(CH32H41X_SPI_CTLR1(base), 0, CH32H41X_SPI_CR1_SPE);

  g_spi1.frequency = frequency;
  return actual;
}

/****************************************************************************
 * Name: ch32h41x_spi_setmode
 *
 * Description:
 *   CPOL/CPHA from the SPI mode number.  Requires SPE off while
 *   changing.
 *
 ****************************************************************************/

static void ch32h41x_spi_setmode(FAR struct spi_dev_s *dev,
                                 enum spi_mode_e mode)
{
  uintptr_t base = SPI1_BASE;
  uint32_t setting;

  if (mode == g_spi1.mode)
    {
      return;
    }

  /* mode 0: CPOL=0 CPHA=0; 1: CPOL=0 CPHA=1; 2: CPOL=1 CPHA=0;
   * 3: CPOL=1 CPHA=1 */

  setting = (uint32_t)mode & 3ul;

  modifyreg32(CH32H41X_SPI_CTLR1(base), CH32H41X_SPI_CR1_SPE, 0);
  modifyreg32(CH32H41X_SPI_CTLR1(base),
              CH32H41X_SPI_CR1_CPOL | CH32H41X_SPI_CR1_CPHA, setting);
  modifyreg32(CH32H41X_SPI_CTLR1(base), 0, CH32H41X_SPI_CR1_SPE);

  g_spi1.mode = mode;
}

/****************************************************************************
 * Name: ch32h41x_spi_setbits
 *
 * Description:
 *   Frame width: 8 (DFF=0) or 16 (DFF=1).  Anything else is rejected.
 *
 ****************************************************************************/

static void ch32h41x_spi_setbits(FAR struct spi_dev_s *dev, int nbits)
{
  uintptr_t base = SPI1_BASE;

  if (nbits != 8 && nbits != 16)
    {
      DEBUGASSERT(nbits == 8 || nbits == 16);
      return;
    }

  if (nbits == g_spi1.nbits)
    {
      return;
    }

  modifyreg32(CH32H41X_SPI_CTLR1(base), CH32H41X_SPI_CR1_SPE, 0);
  modifyreg32(CH32H41X_SPI_CTLR1(base), CH32H41X_SPI_CR1_DFF,
              (nbits == 16) ? CH32H41X_SPI_CR1_DFF : 0);
  modifyreg32(CH32H41X_SPI_CTLR1(base), 0, CH32H41X_SPI_CR1_SPE);

  g_spi1.nbits = nbits;
}

/****************************************************************************
 * Name: ch32h41x_spi_select / status / cmddata / hwfeatures / setdelay
 *
 * Description:
 *   Minimal stubs: software NSS is driven through SSM|SSI inside the
 *   controller so no CS GPIO is toggled here; a CS GPIO can be added
   * per-devid when a real peripheral is attached.
 *
 ****************************************************************************/

static void ch32h41x_spi_select(FAR struct spi_dev_s *dev, uint32_t devid,
                                bool selected)
{
}

static uint8_t ch32h41x_spi_status(FAR struct spi_dev_s *dev,
                                   uint32_t devid)
{
  return 0;
}

/****************************************************************************
 * Name: ch32h41x_spi_send / exchange
 *
 * Description:
 *   Polled full-duplex transfer.  Every byte is clocked by writing
 *   DATAR (8-bit access with DFF=0) and reading the matching RX byte
 *   once RXNE sets.  Bounded waits: a stuck SPI returns -ETIMEDOUT
 *   instead of hanging the caller.
 *
 ****************************************************************************/

static uint32_t ch32h41x_spi_send(FAR struct spi_dev_s *dev, uint32_t wd)
{
  uintptr_t base = SPI1_BASE;
  uint32_t timeout = SPI_SPINLIMIT;

  while ((getreg32(CH32H41X_SPI_STATR(base)) & CH32H41X_SPI_SR_TXE) == 0)
    {
      if (--timeout == 0)
        {
          return 0xffff;
        }
    }

  putreg16((uint16_t)(wd & 0xfful), CH32H41X_SPI_DATAR(base));

  timeout = SPI_SPINLIMIT;
  while ((getreg32(CH32H41X_SPI_STATR(base)) & CH32H41X_SPI_SR_RXNE) == 0)
    {
      if (--timeout == 0)
        {
          return 0xffff;
        }
    }

  return getreg16(CH32H41X_SPI_DATAR(base)) & 0xfful;
}

static void ch32h41x_spi_exchange(FAR struct spi_dev_s *dev,
                                  FAR const void *txbuffer, FAR void *rxbuffer,
                                  size_t nwords)
{
  FAR const uint8_t *tx = (FAR const uint8_t *)txbuffer;
  FAR uint8_t *rx = (FAR uint8_t *)rxbuffer;
  size_t i;

  for (i = 0; i < nwords; i++)
    {
      uint32_t wd = ch32h41x_spi_send(dev, (tx != NULL) ? tx[i] : 0xfful);

      if (rx != NULL)
        {
          rx[i] = (uint8_t)wd;
        }
    }
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The ops members follow the build's Kconfig: SPI_EXCHANGE on,
 * SPI_DELAY_CONTROL / SPI_HWFEATURES / SPI_CMDDATA / SPI_TRIGGER off. */

static const struct spi_ops_s g_spi1_ops =
{
  .lock          = ch32h41x_spi_lock,
  .select        = ch32h41x_spi_select,
  .setfrequency  = ch32h41x_spi_setfrequency,
  .setmode       = ch32h41x_spi_setmode,
  .setbits       = ch32h41x_spi_setbits,
  .status        = ch32h41x_spi_status,
  .send          = ch32h41x_spi_send,
  .exchange      = ch32h41x_spi_exchange,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_spi1_initialize
 *
 * Description:
 *   Bring up SPI1 as master (8 bits, 1MHz-ish default) and register it
 *   as /dev/spi1 (spitool -b 1).
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int ch32h41x_spi1_initialize(void)
{
  static struct spi_dev_s g_spidev;
  static bool registered;
  uintptr_t base = SPI1_BASE;

  if (registered)
    {
      return OK;
    }

  /* Bus clock: SPI1 lives on the HB2 bus (RM HB2PCENR bit12 SPI1EN) */

  modifyreg32(CH32H41X_RCC_HB2PCENR, 0, CH32H41X_RCC_HB2_SPI1);

  /* Pins, from the CH32H417DS0 pin table (all AF5):
   * PA5 SCK/PA7 MOSI = AF push-pull; PA6 MISO = input floating */

  ch32h41x_gpio_setmode(0, 5, CH32H41X_GPIO_MODE_AF_PP_50);
  ch32h41x_gpio_setaf(0, 5, SPI1_AF);
  ch32h41x_gpio_setmode(0, 7, CH32H41X_GPIO_MODE_AF_PP_50);
  ch32h41x_gpio_setaf(0, 7, SPI1_AF);
  ch32h41x_gpio_setmode(0, 6, CH32H41X_GPIO_MODE_IN_FLOAT);

  /* Master, software NSS (SSM|SSI keeps MODF away), 8-bit frames.
   * Start with SPE off; the setfrequency/setmode helpers re-enable. */

  putreg32(CH32H41X_SPI_CR1_MSTR | CH32H41X_SPI_CR1_SSM |
           CH32H41X_SPI_CR1_SSI | CH32H41X_SPI_CR1_BR(5),
           CH32H41X_SPI_CTLR1(base));
  putreg32(0, CH32H41X_SPI_CTLR2(base));

  g_spi1.frequency = 0;
  g_spi1.mode = (enum spi_mode_e)-1;
  g_spi1.nbits = 0;
  nxsem_init(&g_spi1.lock, 0, 1);

  g_spidev.ops = &g_spi1_ops;

  int ret = spi_register(&g_spidev, 1);
  if (ret != OK)
    {
      return ret;
    }

  registered = true;
  return OK;
}
