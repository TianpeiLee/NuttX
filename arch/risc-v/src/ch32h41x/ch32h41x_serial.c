/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_serial.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/serial/serial.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_usart.h"

#include "ch32h41x_internal.h"
#include "ch32h41x_config.h"
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef USE_SERIALDRIVER

/* Which UART is the console?  The console is always registered as
 * /dev/console; the remaining UARTs are /dev/ttyS<n-1>.
 */

#ifdef HAVE_SERIAL_CONSOLE
#  define CONSOLE_DEV CH32H41X_CONCAT(g_uart, \
                                      CH32H41X_CONCAT(CONSOLE_UART, port))
#endif

#ifdef HAVE_UART_DEVICE

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Per-UART static configuration (this NuttX tree has no public
 * struct uart_config_s; the values come from the CONFIG_UART<n>_*
 * configuration items).
 */

struct ch32h41x_uart_config_s
{
  uint32_t baud;        /* configured baud rate */
  uint8_t parity;       /* 0=none, 1=odd, 2=even */
  uint8_t bits;         /* word length, 8 or 9 */
  uint8_t stop_bits2;   /* two stop bits */
};

struct ch32h41x_uart_s
{
  uint8_t idx;              /* USART number 1..8 */
  uint8_t irq;              /* NuttX IRQ number */
  struct ch32h41x_uart_config_s config;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int ch32h41x_setup(struct uart_dev_s *dev);
static void ch32h41x_shutdown(struct uart_dev_s *dev);
static int ch32h41x_attach(struct uart_dev_s *dev);
static void ch32h41x_detach(struct uart_dev_s *dev);
static int ch32h41x_ioctl(struct file *filep, int cmd, unsigned long arg);
static int ch32h41x_receive(struct uart_dev_s *dev, unsigned int *status);
static void ch32h41x_rxint(struct uart_dev_s *dev, bool enable);
static bool ch32h41x_rxavailable(struct uart_dev_s *dev);
static void ch32h41x_send(struct uart_dev_s *dev, int ch);
static void ch32h41x_txint(struct uart_dev_s *dev, bool enable);
static bool ch32h41x_txready(struct uart_dev_s *dev);
static bool ch32h41x_txempty(struct uart_dev_s *dev);
static int ch32h41x_serial_interrupt(int irq, void *context, void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uintptr_t g_usart_base[9] =
{
  0,
  CH32H41X_USART1_BASE, CH32H41X_USART2_BASE, CH32H41X_USART3_BASE,
  CH32H41X_USART4_BASE, CH32H41X_USART5_BASE, CH32H41X_USART6_BASE,
  CH32H41X_USART7_BASE, CH32H41X_USART8_BASE,
};

static const struct uart_ops_s g_uart_ops =
{
  .setup       = ch32h41x_setup,
  .shutdown    = ch32h41x_shutdown,
  .attach      = ch32h41x_attach,
  .detach      = ch32h41x_detach,
  .ioctl       = ch32h41x_ioctl,
  .receive     = ch32h41x_receive,
  .rxint       = ch32h41x_rxint,
  .rxavailable = ch32h41x_rxavailable,
#ifdef CONFIG_SERIAL_IFLOWCONTROL
  .rxflowcontrol = NULL,
#endif
  .send    = ch32h41x_send,
  .txint   = ch32h41x_txint,
  .txready = ch32h41x_txready,
  .txempty = ch32h41x_txempty,
};

/****************************************************************************
 * Private Data: per-UART state
 *
 * USART n uses the standard CONFIG_UART<n>_* configuration items; USART1
 * is /dev/ttyS0, USART2 is /dev/ttyS1, and so on.
 ****************************************************************************/

#define CH32H41X_CAT2(a, b)   a##b
#define CH32H41X_CAT3(a, b, c)  a##b##c
#define CH32H41X_CONCAT(a, b) CH32H41X_CAT2(a, b)

#define CH32H41X_UART_DECL(n) \
  static char g_uart##n##rxbuffer[CONFIG_UART##n##_RXBUFSIZE]; \
  static char g_uart##n##txbuffer[CONFIG_UART##n##_TXBUFSIZE]; \
  static struct ch32h41x_uart_s g_uart##n##priv = \
  { \
    .idx      = n, \
    .irq      = CH32H41X_CAT3(CH32H41X_IRQ_USART, n, ), \
    .config   = \
    { \
      .baud       = CONFIG_UART##n##_BAUD, \
      .parity     = CONFIG_UART##n##_PARITY, \
      .bits       = CONFIG_UART##n##_BITS, \
      .stop_bits2 = CONFIG_UART##n##_2STOP, \
    }, \
  }; \
  static uart_dev_t g_uart##n##port = \
  { \
    .recv = \
    { \
      .size   = CONFIG_UART##n##_RXBUFSIZE, \
      .buffer = g_uart##n##rxbuffer, \
    }, \
    .xmit = \
    { \
      .size   = CONFIG_UART##n##_TXBUFSIZE, \
      .buffer = g_uart##n##txbuffer, \
    }, \
    .ops  = &g_uart_ops, \
    .priv = (void *)&g_uart##n##priv, \
  };

#ifdef CONFIG_CH32H41X_USART1
CH32H41X_UART_DECL(1)
#  define UART1_DEV_PTR &g_uart1port
#else
#  define UART1_DEV_PTR NULL
#endif

#ifdef CONFIG_CH32H41X_USART2
CH32H41X_UART_DECL(2)
#  define UART2_DEV_PTR &g_uart2port
#else
#  define UART2_DEV_PTR NULL
#endif

#ifdef CONFIG_CH32H41X_USART3
CH32H41X_UART_DECL(3)
#  define UART3_DEV_PTR &g_uart3port
#else
#  define UART3_DEV_PTR NULL
#endif

#ifdef CONFIG_CH32H41X_USART4
CH32H41X_UART_DECL(4)
#  define UART4_DEV_PTR &g_uart4port
#else
#  define UART4_DEV_PTR NULL
#endif

#ifdef CONFIG_CH32H41X_USART5
CH32H41X_UART_DECL(5)
#  define UART5_DEV_PTR &g_uart5port
#else
#  define UART5_DEV_PTR NULL
#endif

#ifdef CONFIG_CH32H41X_USART6
CH32H41X_UART_DECL(6)
#  define UART6_DEV_PTR &g_uart6port
#else
#  define UART6_DEV_PTR NULL
#endif

#ifdef CONFIG_CH32H41X_USART7
CH32H41X_UART_DECL(7)
#  define UART7_DEV_PTR &g_uart7port
#else
#  define UART7_DEV_PTR NULL
#endif

#ifdef CONFIG_CH32H41X_USART8
CH32H41X_UART_DECL(8)
#  define UART8_DEV_PTR &g_uart8port
#else
#  define UART8_DEV_PTR NULL
#endif

static uart_dev_t *g_uart_devs[8] =
{
  UART1_DEV_PTR, UART2_DEV_PTR, UART3_DEV_PTR, UART4_DEV_PTR,
  UART5_DEV_PTR, UART6_DEV_PTR, UART7_DEV_PTR, UART8_DEV_PTR,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_uart_brr
 *
 * Description:
 *   Compute the baud rate divider for a USART clocked by HCLK, using the
 *   fixed-point algorithm from the WCH SDK (oversampling by 16).
 *
 ****************************************************************************/

static uint16_t ch32h41x_uart_brr(uint32_t baud)
{
  uint32_t hbclock = ch32h41x_hclk_frequency();
  uint32_t div = (25 * hbclock) / (4 * baud);
  uint32_t mantissa = (div / 100) << 4;
  uint32_t frac = div - 100 * (mantissa >> 4);

  frac = ((frac * 16) + 50) / 100;
  if (frac > 0xf)
    {
      mantissa += 1 << 4;
    }
  else
    {
      mantissa |= frac;
    }

  return (uint16_t)mantissa;
}

/****************************************************************************
 * Name: ch32h41x_setup
 *
 * Description:
 *   Configure the UART for the buffered, interrupt driven operation.
 *
 ****************************************************************************/

static int ch32h41x_setup(struct uart_dev_s *dev)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];
  uint32_t cr1;

  /* Disable the UART while it is being configured */

  modreg16(0, CH32H41X_USART_CR1_UE, base + CH32H41X_USART_CTLR1);

  /* Program the baud rate */

  putreg16(ch32h41x_uart_brr(priv->config.baud),
           base + CH32H41X_USART_BRR);

  /* Word length, parity and mode */

  cr1 = CH32H41X_USART_CR1_UE |
        CH32H41X_USART_CR1_TE |
        CH32H41X_USART_CR1_RE;

  if (priv->config.bits == 9)
    {
      cr1 |= CH32H41X_USART_CR1_M;
    }

  if (priv->config.parity != 0)
    {
      cr1 |= CH32H41X_USART_CR1_PCE;
      if (priv->config.parity == 1)   /* 1 = odd */
        {
          cr1 |= CH32H41X_USART_CR1_PS;
        }
    }

  putreg16((uint16_t)cr1, base + CH32H41X_USART_CTLR1);

  /* Stop bits */

  if (priv->config.stop_bits2)
    {
      putreg16(CH32H41X_USART_CR2_STOP_2, base + CH32H41X_USART_CTLR2);
    }
  else
    {
      putreg16(CH32H41X_USART_CR2_STOP_1, base + CH32H41X_USART_CTLR2);
    }

  /* Clear any latched status flags */

  putreg16(0, base + CH32H41X_USART_STATR);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_shutdown
 ****************************************************************************/

static void ch32h41x_shutdown(struct uart_dev_s *dev)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;

  modreg16(0, CH32H41X_USART_CR1_UE,
           g_usart_base[priv->idx] + CH32H41X_USART_CTLR1);
}

/****************************************************************************
 * Name: ch32h41x_attach / ch32h41x_detach
 ****************************************************************************/

static int ch32h41x_attach(struct uart_dev_s *dev)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  int ret;

  ret = irq_attach(priv->irq, ch32h41x_serial_interrupt, dev);
  if (ret == OK)
    {
      up_enable_irq(priv->irq);
    }

  return ret;
}

static void ch32h41x_detach(struct uart_dev_s *dev)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;

  up_disable_irq(priv->irq);
}

/****************************************************************************
 * Name: ch32h41x_ioctl
 ****************************************************************************/

static int ch32h41x_ioctl(struct file *filep, int cmd, unsigned long arg)
{
  return -ENOTTY;
}

/****************************************************************************
 * Name: ch32h41x_receive
 *
 * Description:
 *   Read one character.  Reading DATAR after reading STATR clears the
 *   latched error flags (including overrun).
 *
 ****************************************************************************/

static int ch32h41x_receive(struct uart_dev_s *dev, unsigned int *status)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];
  uint16_t statr = getreg16(base + CH32H41X_USART_STATR);
  int rxdata;

  if (status)
    {
      *status = statr & CH32H41X_USART_SR_ERROR_MASK;
    }

  rxdata = getreg16(base + CH32H41X_USART_DATAR) & 0xff;
  return rxdata;
}

/****************************************************************************
 * Name: ch32h41x_rxint
 ****************************************************************************/

static void ch32h41x_rxint(struct uart_dev_s *dev, bool enable)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];
  irqstate_t flags;

  flags = enter_critical_section();
  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      modreg16(CH32H41X_USART_CR1_RXNEIE,
               CH32H41X_USART_CR1_RXNEIE,
               base + CH32H41X_USART_CTLR1);
#endif
    }
  else
    {
      modreg16(0, CH32H41X_USART_CR1_RXNEIE,
               base + CH32H41X_USART_CTLR1);
    }

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: ch32h41x_rxavailable
 ****************************************************************************/

static bool ch32h41x_rxavailable(struct uart_dev_s *dev)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];

  return (getreg16(base + CH32H41X_USART_STATR) &
          CH32H41X_USART_SR_RXNEF) != 0;
}

/****************************************************************************
 * Name: ch32h41x_send
 ****************************************************************************/

static void ch32h41x_send(struct uart_dev_s *dev, int ch)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];

  while ((getreg16(base + CH32H41X_USART_STATR) &
          CH32H41X_USART_SR_TXEF) == 0)
    {
    }

  putreg16((uint16_t)ch, base + CH32H41X_USART_DATAR);
}

/****************************************************************************
 * Name: ch32h41x_txint
 ****************************************************************************/

static void ch32h41x_txint(struct uart_dev_s *dev, bool enable)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];
  irqstate_t flags;

  flags = enter_critical_section();
  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      modreg16(CH32H41X_USART_CR1_TXEIE,
               CH32H41X_USART_CR1_TXEIE,
               base + CH32H41X_USART_CTLR1);

      /* Fake a TX ready interrupt in case the data register is already
       * empty and no interrupt would be raised.
       */

      uart_xmitchars(dev);
#endif
    }
  else
    {
      modreg16(0, CH32H41X_USART_CR1_TXEIE,
               base + CH32H41X_USART_CTLR1);
    }

  leave_critical_section(flags);
}

/****************************************************************************
 * Name: ch32h41x_txready
 ****************************************************************************/

static bool ch32h41x_txready(struct uart_dev_s *dev)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];

  return (getreg16(base + CH32H41X_USART_STATR) &
          CH32H41X_USART_SR_TXEF) != 0;
}

/****************************************************************************
 * Name: ch32h41x_txempty
 ****************************************************************************/

static bool ch32h41x_txempty(struct uart_dev_s *dev)
{
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];

  return (getreg16(base + CH32H41X_USART_STATR) &
          CH32H41X_USART_SR_TCF) != 0;
}

/****************************************************************************
 * Name: ch32h41x_serial_interrupt
 ****************************************************************************/

static int ch32h41x_serial_interrupt(int irq, void *context, void *arg)
{
  struct uart_dev_s *dev = (struct uart_dev_s *)arg;
  struct ch32h41x_uart_s *priv = (struct ch32h41x_uart_s *)dev->priv;
  uintptr_t base = g_usart_base[priv->idx];
  uint16_t statr = getreg16(base + CH32H41X_USART_STATR);

  /* Receive data available (or receive error to drain): read DATAR to
   * clear the latched flags.
   */

  if ((statr & (CH32H41X_USART_SR_RXNEF | CH32H41X_USART_SR_ERROR_MASK)) != 0)
    {
      uart_recvchars(dev);
    }

  /* Transmit data register empty */

  if ((statr & CH32H41X_USART_SR_TXEF) != 0)
    {
      uart_xmitchars(dev);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef USE_EARLYSERIALINIT

/****************************************************************************
 * Name: riscv_earlyserialinit
 *
 * Description:
 *   Performs the low level UART initialization early in debug so that the
 *   serial console will be available during boot up.  This must be called
 *   before riscv_serialinit.
 *
 ****************************************************************************/

void riscv_earlyserialinit(void)
{
#ifdef HAVE_SERIAL_CONSOLE
  /* Configure whichever UART is the console */

  CONSOLE_DEV.isconsole = true;
  ch32h41x_setup(&CONSOLE_DEV);
#endif
}

#endif /* USE_EARLYSERIALINIT */

/****************************************************************************
 * Name: riscv_serialinit
 *
 * Description:
 *   Register serial console and serial ports.  This assumes that
 *   riscv_earlyserialinit was called previously.
 *
 ****************************************************************************/

void riscv_serialinit(void)
{
  int i;
  char devname[16];

#ifdef HAVE_SERIAL_CONSOLE
  /* Register the console */

  uart_register("/dev/console", &CONSOLE_DEV);
#endif

  /* Register all remaining UARTs as /dev/ttyS<n-1> */

  strlcpy(devname, "/dev/ttySx", sizeof(devname));
  for (i = 0; i < 8; i++)
    {
      if (g_uart_devs[i] == NULL)
        {
          continue;
        }

      /* Don't create a device for the console - we did that above */

      if (g_uart_devs[i]->isconsole)
        {
          continue;
        }

      devname[9] = '0' + i;
      uart_register(devname, g_uart_devs[i]);
    }
}

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug writes.
 *
 ****************************************************************************/

void up_putc(int ch)
{
#ifdef HAVE_SERIAL_CONSOLE
  irqstate_t flags = up_irq_save();

  riscv_lowputc(ch);
  if ((ch == '\n'))
    {
      riscv_lowputc('\r');
    }

  up_irq_restore(flags);
#endif
}

#else /* HAVE_UART_DEVICE */

/****************************************************************************
 * Name: riscv_earlyserialinit, riscv_serialinit, and up_putc
 *
 * Description:
 *   Stubs used when all UARTs are disabled.
 *
 ****************************************************************************/

void riscv_earlyserialinit(void)
{
}

void riscv_serialinit(void)
{
}

void up_putc(int ch)
{
}

#endif /* HAVE_UART_DEVICE */

#else /* USE_SERIALDRIVER */

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug writes.
 *
 ****************************************************************************/

void up_putc(int ch)
{
#ifdef HAVE_SERIAL_CONSOLE
  riscv_lowputc(ch);
  if ((ch == '\n'))
    {
      riscv_lowputc('\r');
    }
#endif
}

#endif /* USE_SERIALDRIVER */
