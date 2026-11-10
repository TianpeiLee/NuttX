/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_clockconfig.c
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

/* Clock bring-up sequences derived from the WCH CH32H417 EVT
 * system_ch32h417.c (V1.0.8, Apache-2.0 licensed vendor code).
 *
 * Clock tree (per the EVT RCC_GetClocksFreq decoding):
 *   V5F core clock = SYSCLK / HPRE
 *   HCLK           = (SYSCLK / HPRE) / FPRE
 *   V3F core clock = HCLK
 *   Peripheral clocks = HCLK
 *
 * The chip has several PLLs: the main SYSPLL can be driven by the
 * SYSPLL (25MHz * multiplier) or by one of the dedicated PLLs (USBHS
 * 480MHz, ETH 500MHz, SerDes, USBSS 125MHz).
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <assert.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"

#include "ch32h41x_internal.h"
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CH32H41X_HSI_FREQUENCY   25000000ul
#define CH32H41X_HSE_FREQUENCY   CONFIG_CH32H41X_HSE_VALUE

#define HSE_STARTUP_TIMEOUT      0xffff

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile uint32_t g_ch32h41x_sysclk = CH32H41X_HSI_FREQUENCY;
static volatile uint32_t g_ch32h41x_hclk = CH32H41X_HSI_FREQUENCY;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_hse_ready
 *
 * Description:
 *   Enable the HSE oscillator and wait for it to become ready.  Returns
 *   true on success.
 *
 ****************************************************************************/

static bool ch32h41x_hse_start(void)
{
  uint32_t timeout = HSE_STARTUP_TIMEOUT;

  modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_HSEON);

  while ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_HSERDY) == 0)
    {
      if (--timeout == 0)
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: ch32h41x_switch_sysclk
 *
 * Description:
 *   Gate the SYSPLL output on and switch the system clock source to it.
 *   Returns false if the switch does not complete (caller keeps running
 *   from the HSI).
 *
 ****************************************************************************/

static bool ch32h41x_switch_sysclk(void)
{
  uint32_t timeout = HSE_STARTUP_TIMEOUT;

  /* Select the flash latency for the HCLK frequency */

  modifyreg32(CH32H41X_FLASH_ACTLR,
              CH32H41X_FLASH_LATENCY_MASK, CH32H41X_FLASH_LATENCY_DIV2);

  /* Gate the SYSPLL output on and select it as system clock.  Bounded
   * wait: on failure the caller keeps running from the HSI.
   */

  modifyreg32(CH32H41X_RCC_PLLCFGR, 0, CH32H41X_RCC_SYSPLL_GATE);
  modifyreg32(CH32H41X_RCC_CFGR0, CH32H41X_RCC_SW_MASK, CH32H41X_RCC_SW_PLL);

  while ((getreg32(CH32H41X_RCC_CFGR0) & CH32H41X_RCC_SWS_MASK) !=
         CH32H41X_RCC_SWS_PLL)
    {
      if (--timeout == 0)
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: ch32h41x_setclock_480m
 *
 * Description:
 *   SYSCLK = 480MHz from the USBHS PLL (25MHz HSE reference).  V5F core
 *   clock = SYSCLK/HPRE = 240MHz; HCLK = SYSCLK/HPRE/FPRE = 120MHz;
 *   V3F core clock = HCLK = 120MHz; peripheral clock = HCLK.  This is
 *   the recommended configuration when USBHS or USBSS will be used
 *   later, and it is the EVT default 480M option.
 *
 ****************************************************************************/

#ifdef CONFIG_CH32H41X_SYSCLK_480MHZ
static void ch32h41x_setclock_480m(void)
{
  uint32_t timeout = HSE_STARTUP_TIMEOUT;

  if (!ch32h41x_hse_start())
    {
      return;
    }

  /* USBHS PLL: HSE reference at 25MHz */

  modifyreg32(CH32H41X_RCC_PLLCFGR2,
              CH32H41X_RCC_USBHSPLL_REFSEL_MASK |
              CH32H41X_RCC_USBHSPLLSRC,
              CH32H41X_RCC_USBHSPLL_REFSEL_25MHZ |
              CH32H41X_RCC_USBHSPLLSRC_HSE);

  /* Enable the USBHS PLL and wait for lock (bounded) */

  modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_USBHS_PLLON);
  while ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_USBHS_PLLRDY) == 0)
    {
      if (--timeout == 0)
        {
          return;
        }
    }

  /* Use the USBHS PLL as the SYSPLL source (output still gated) */

  modifyreg32(CH32H41X_RCC_PLLCFGR,
              CH32H41X_RCC_SYSPLL_SEL_MASK | CH32H41X_RCC_SYSPLL_GATE,
              CH32H41X_RCC_SYSPLL_SEL_USBHS);

  /* V5F core = SYSCLK/2 = 240MHz, HCLK = V3F = 240MHz/2 = 120MHz */

  modifyreg32(CH32H41X_RCC_CFGR0,
              CH32H41X_RCC_HPRE_MASK | CH32H41X_RCC_FPRE_MASK,
              CH32H41X_RCC_HPRE_DIV2 | CH32H41X_RCC_FPRE_DIV2);

  if (!ch32h41x_switch_sysclk())
    {
      return;
    }

  g_ch32h41x_sysclk = 480000000ul;
  g_ch32h41x_hclk   = 120000000ul;
}
#endif

/****************************************************************************
 * Name: ch32h41x_setclock_400m
 *
 * Description:
 *   SYSCLK = 400MHz from the main SYSPLL (25MHz * 16).  V5F core clock
 *   = SYSCLK/HPRE = 400MHz; HCLK = SYSCLK/HPRE/FPRE = 100MHz; V3F core
 *   clock = HCLK = 100MHz; peripheral clock = HCLK.  This is the EVT
 *   default 400M option.
 *
 ****************************************************************************/

#ifdef CONFIG_CH32H41X_SYSCLK_400MHZ
static void ch32h41x_setclock_400m(void)
{
  uint32_t timeout = HSE_STARTUP_TIMEOUT;

  if (!ch32h41x_hse_start())
    {
      return;
    }

  /* SYSPLL: HSE / 1 * 16 = 400MHz */

  modifyreg32(CH32H41X_RCC_PLLCFGR,
              CH32H41X_RCC_PLLMUL_MASK |
              CH32H41X_RCC_PLL_SRCDIV_MASK |
              CH32H41X_RCC_PLLSRC,
              CH32H41X_RCC_PLLMUL16 |
              CH32H41X_RCC_PLL_SRCDIV_DIV1 |
              CH32H41X_RCC_PLLSRC_HSE);

  /* Enable the SYSPLL and wait for lock (bounded) */

  modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_PLLON);
  while ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_PLLRDY) == 0)
    {
      if (--timeout == 0)
        {
          return;
        }
    }

  /* Use the SYSPLL as the SYSPLL source (output still gated) */

  modifyreg32(CH32H41X_RCC_PLLCFGR,
              CH32H41X_RCC_SYSPLL_SEL_MASK | CH32H41X_RCC_SYSPLL_GATE,
              CH32H41X_RCC_SYSPLL_SEL_PLL);

  /* V5F core = SYSCLK/1 = 400MHz, HCLK = V3F = 400MHz/4 = 100MHz */

  modifyreg32(CH32H41X_RCC_CFGR0,
              CH32H41X_RCC_HPRE_MASK | CH32H41X_RCC_FPRE_MASK,
              CH32H41X_RCC_HPRE_DIV1 | CH32H41X_RCC_FPRE_DIV4);

  if (!ch32h41x_switch_sysclk())
    {
      return;
    }

  g_ch32h41x_sysclk = 400000000ul;
  g_ch32h41x_hclk   = 100000000ul;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_sysclk_frequency / ch32h41x_hclk_frequency
 ****************************************************************************/

uint32_t ch32h41x_sysclk_frequency(void)
{
  return g_ch32h41x_sysclk;
}

uint32_t ch32h41x_hclk_frequency(void)
{
  return g_ch32h41x_hclk;
}

/****************************************************************************
 * Name: ch32h41x_clockconfig
 *
 * Description:
 *   Called early in boot to bring the clocks up.
 *
 *   V3F (boot master): reset defaults run from the 25MHz HSI.  Clear the
 *   pending ready flags, then apply the selected SYSCLK configuration.
 *   On HSE failure the chip keeps running from the HSI (a console banner
 *   will show reduced baud accuracy) instead of hanging.
 *
 *   V5F: the V3F has already configured the clocks before waking this
 *   core; just record the configured frequencies.
 *
 ****************************************************************************/

void ch32h41x_clockconfig(void)
{
  /* Clear all pending clock-ready interrupt flags */

  putreg32(CH32H41X_RCC_ALLFLAGS, CH32H41X_RCC_INTR);

#if defined(CH32H41X_CORE_V3F)
  {
#if defined(CONFIG_CH32H41X_SYSCLK_480MHZ)
    ch32h41x_setclock_480m();
#elif defined(CONFIG_CH32H41X_SYSCLK_400MHZ)
    ch32h41x_setclock_400m();
#endif
  }
#else /* CH32H41X_CORE_V5F */
  {
    /* The boot master core has configured the clocks.  Derive the
     * frequencies from the configuration so the drivers can program
     * dividers without reading the RCC registers.
     */

#if defined(CONFIG_CH32H41X_SYSCLK_480MHZ)
    g_ch32h41x_sysclk = 480000000ul;
    g_ch32h41x_hclk   = 120000000ul;
#elif defined(CONFIG_CH32H41X_SYSCLK_400MHZ)
    g_ch32h41x_sysclk = 400000000ul;
    g_ch32h41x_hclk   = 100000000ul;
#endif
  }
#endif
}
