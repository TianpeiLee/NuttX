/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_lowputc.c
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

#include <stdint.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_gpio.h"
#include "hardware/ch32h41x_usart.h"

#include "ch32h41x_internal.h"
#include "ch32h41x_config.h"
#include "ch32h41x_gpio.h"
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* UART instance bases and clock gates, indexed 1..8 (USART1..USART8).
 * USART1 lives on the HB2 bus together with the GPIO/AFIO blocks; the
 * remaining UARTs are clocked from HB1.
 */

static const uintptr_t g_usart_base[9] =
{
  0,
  CH32H41X_USART1_BASE, CH32H41X_USART2_BASE, CH32H41X_USART3_BASE,
  CH32H41X_USART4_BASE, CH32H41X_USART5_BASE, CH32H41X_USART6_BASE,
  CH32H41X_USART7_BASE, CH32H41X_USART8_BASE,
};

struct ch32h41x_uart_gate_s
{
  uintptr_t reg;
  uint32_t mask;
};

static const struct ch32h41x_uart_gate_s g_usart_gate[9] =
{
  { 0, 0 },
  { CH32H41X_RCC_HB2PCENR, CH32H41X_RCC_HB2_USART1 },
  { CH32H41X_RCC_HB1PCENR, CH32H41X_RCC_HB1_USART2 },
  { CH32H41X_RCC_HB1PCENR, CH32H41X_RCC_HB1_USART3 },
  { CH32H41X_RCC_HB1PCENR, CH32H41X_RCC_HB1_USART4 },
  { CH32H41X_RCC_HB1PCENR, CH32H41X_RCC_HB1_USART5 },
  { CH32H41X_RCC_HB1PCENR, CH32H41X_RCC_HB1_USART6 },
  { CH32H41X_RCC_HB1PCENR, CH32H41X_RCC_HB1_USART7 },
  { CH32H41X_RCC_HB1PCENR, CH32H41X_RCC_HB1_USART8 },
};

#ifdef HAVE_SERIAL_CONSOLE

/* Token pasting helpers for the console configuration macros */

#define CH32H41X_CAT2(a, b)       a##b
#define CH32H41X_CONCAT(a, b)     CH32H41X_CAT2(a, b)
#define CONSOLE_BAUD              CH32H41X_CONCAT(CONFIG_UART, \
                                                  CH32H41X_CONCAT(CONSOLE_UART, _BAUD))

#  define CONSOLE_BASE g_usart_base[CONSOLE_UART]
#endif

/* Configure the board-defined pin pair for USART n (macros from board.h) */

#define CH32H41X_UART_PINS(n) \
  do \
  { \
    ch32h41x_gpio_setmode(GPIO_USART##n##_TX_PORT, GPIO_USART##n##_TX_PIN, \
                          CH32H41X_GPIO_MODE_AF_PP_50); \
    ch32h41x_gpio_setaf(GPIO_USART##n##_TX_PORT, GPIO_USART##n##_TX_PIN, \
                        GPIO_USART##n##_TX_AF); \
    ch32h41x_gpio_write(GPIO_USART##n##_RX_PORT, GPIO_USART##n##_RX_PIN, \
                        true); \
    ch32h41x_gpio_setmode(GPIO_USART##n##_RX_PORT, GPIO_USART##n##_RX_PIN, \
                          CH32H41X_GPIO_MODE_IN_PULL); \
    ch32h41x_gpio_setaf(GPIO_USART##n##_RX_PORT, GPIO_USART##n##_RX_PIN, \
                        GPIO_USART##n##_RX_AF); \
  } while (0)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_usart_brr
 *
 * Description:
 *   Compute the baud rate divider for a USART clocked by HCLK, using the
 *   fixed-point algorithm from the WCH SDK (oversampling by 16).
 *
 ****************************************************************************/

static uint16_t ch32h41x_usart_brr(uint32_t baud)
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: riscv_lowputc
 *
 * Description:
 *   Write one character to the console UART (polled).
 *
 ****************************************************************************/

void riscv_lowputc(char ch)
{
#ifdef HAVE_SERIAL_CONSOLE
  while ((getreg16(CONSOLE_BASE + CH32H41X_USART_STATR) &
          CH32H41X_USART_SR_TXEF) == 0)
    {
    }

  putreg16((uint16_t)ch, CONSOLE_BASE + CH32H41X_USART_DATAR);
#endif
}

/****************************************************************************
 * Name: ch32h41x_lowsetup
 *
 * Description:
 *   Configure the console UART for polled output early in boot: enable
 *   the peripheral clocks and the board-defined pins for every enabled
 *   UART, and program the console baud rate.
 *
 ****************************************************************************/

void ch32h41x_lowsetup(void)
{
  /* The AFIO/GPIO clocks are required for the pin configuration below */

  ch32h41x_gpioinit();

#if defined(CONFIG_CH32H41X_USART1) && defined(GPIO_USART1_TX_PIN)
  CH32H41X_UART_PINS(1);
#endif
#if defined(CONFIG_CH32H41X_USART2) && defined(GPIO_USART2_TX_PIN)
  CH32H41X_UART_PINS(2);
#endif
#if defined(CONFIG_CH32H41X_USART3) && defined(GPIO_USART3_TX_PIN)
  CH32H41X_UART_PINS(3);
#endif
#if defined(CONFIG_CH32H41X_USART4) && defined(GPIO_USART4_TX_PIN)
  CH32H41X_UART_PINS(4);
#endif
#if defined(CONFIG_CH32H41X_USART5) && defined(GPIO_USART5_TX_PIN)
  CH32H41X_UART_PINS(5);
#endif
#if defined(CONFIG_CH32H41X_USART6) && defined(GPIO_USART6_TX_PIN)
  CH32H41X_UART_PINS(6);
#endif
#if defined(CONFIG_CH32H41X_USART7) && defined(GPIO_USART7_TX_PIN)
  CH32H41X_UART_PINS(7);
#endif
#if defined(CONFIG_CH32H41X_USART8) && defined(GPIO_USART8_TX_PIN)
  CH32H41X_UART_PINS(8);
#endif

#ifdef HAVE_SERIAL_CONSOLE
  {
    int n = CONSOLE_UART;

    /* Enable the USART clock */

    modifyreg32(g_usart_gate[n].reg, 0, g_usart_gate[n].mask);

    /* Program the baud rate and enable the UART for polled output */

    putreg16(ch32h41x_usart_brr(CONSOLE_BAUD),
             g_usart_base[n] + CH32H41X_USART_BRR);
    putreg16(CH32H41X_USART_CR1_UE | CH32H41X_USART_CR1_TE |
             CH32H41X_USART_CR1_RE,
             g_usart_base[n] + CH32H41X_USART_CTLR1);
  }
#endif
}
