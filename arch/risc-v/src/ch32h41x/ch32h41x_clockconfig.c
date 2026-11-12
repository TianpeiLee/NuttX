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

/* EVT uses 0x1000000 (a ~16s bound) for the HSE startup wait; that is
 * appropriate for a failing crystal but punitive when the sequence is
 * retried.  One second per bounded wait keeps a cold-boot crystal start
 * covered while bounding a broken-crystal boot to a few seconds. */

#define HSE_STARTUP_TIMEOUT      0x100000

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile uint32_t g_ch32h41x_sysclk = CH32H41X_HSI_FREQUENCY;
static volatile uint32_t g_ch32h41x_hclk = CH32H41X_HSI_FREQUENCY;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_hse_start
 *
 * Description:
 *   Enable the HSE oscillator and wait for it to become ready.  Returns
 *   true on success.
 *
 *   Only used by the 480MHz path; the 400MHz sequence inlines its own
 *   bounded waits per the EVT flow.
 *
 ****************************************************************************/

#ifdef CONFIG_CH32H41X_SYSCLK_480MHZ
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
#endif

/****************************************************************************
 * Name: ch32h41x_switch_sysclk
 *
 * Description:
 *   Gate the SYSPLL output on and switch the system clock source to it.
 *   Returns false if the switch does not complete (caller keeps running
 *   from the HSI).
 *
 ****************************************************************************/

#ifdef CONFIG_CH32H41X_SYSCLK_480MHZ
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
#endif

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
  uint32_t timeout;
  uint32_t reg;
  unsigned retry;

  /* This follows the WCH EVT system_ch32h417.c sequence for
   * SYSCLK_400M_CoreCLK_V5F_400M_V3F_100M_HSE step by step, including
   * its mux-synchronization polls.  During a cold power-on the V5F boot
   * ROM also programs the SYSPLL for its own boot checks and races this
   * code for the RCC registers, so the final register state is verified
   * and the whole sequence is retried when it does not stick. */

  for (retry = 0; retry < 4; retry++)
    {
      /* EVT SystemInit prologue: turn off the PLLs, the HSE and its
       * bypass so the bring-up below starts from a known analog state */

      modifyreg32(CH32H41X_RCC_CTLR, 0x95590000ul, 0);
      modifyreg32(CH32H41X_RCC_CTLR, CH32H41X_RCC_HSEBYP, 0);
      modifyreg32(CH32H41X_RCC_PLLCFGR, CH32H41X_RCC_SYSPLL_GATE, 0);

      /* HSE oscillator on and wait for ready (bounded) */

      modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_HSEON);

      timeout = HSE_STARTUP_TIMEOUT;
      while ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_HSERDY) == 0)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      if ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_HSERDY) == 0)
        {
          continue;               /* HSE did not start; retry */
        }

      /* SYSPLL = HSE * 16 = 400MHz, then wait for the source mux sync */

      modifyreg32(CH32H41X_RCC_PLLCFGR,
                  CH32H41X_RCC_PLLMUL_MASK |
                  CH32H41X_RCC_PLL_SRCDIV_MASK |
                  CH32H41X_RCC_PLLSRC,
                  CH32H41X_RCC_PLLMUL16 |
                  CH32H41X_RCC_PLL_SRCDIV_DIV1 |
                  CH32H41X_RCC_PLLSRC_HSE);

      timeout = HSE_STARTUP_TIMEOUT;
      while ((getreg32(CH32H41X_RCC_PLLCFGR) & CH32H41X_RCC_PLLSRC) !=
             CH32H41X_RCC_PLLSRC_HSE)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      /* PLL on and wait for lock (bounded) */

      modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_PLLON);

      timeout = HSE_STARTUP_TIMEOUT;
      while ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_PLLRDY) == 0)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      if ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_PLLRDY) == 0)
        {
          continue;               /* PLL did not lock; retry */
        }

      /* Deselect and ungate the SYSPLL output, wait for the mux sync */

      modifyreg32(CH32H41X_RCC_PLLCFGR,
                  CH32H41X_RCC_SYSPLL_SEL_MASK | CH32H41X_RCC_SYSPLL_GATE,
                  0);

      timeout = HSE_STARTUP_TIMEOUT;
      while ((getreg32(CH32H41X_RCC_PLLCFGR) &
              CH32H41X_RCC_SYSPLL_SEL_MASK) != 0)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      /* Prescalers: V5F core = SYSCLK/1 = 400MHz,
       * HCLK (V3F + peripherals) = SYSCLK/4 = 100MHz */

      modifyreg32(CH32H41X_RCC_CFGR0,
                  CH32H41X_RCC_HPRE_MASK | CH32H41X_RCC_FPRE_MASK,
                  CH32H41X_RCC_HPRE_DIV1 | CH32H41X_RCC_FPRE_DIV4);

      /* Flash latency for the new HCLK, then gate the SYSPLL output in
       * (the EVT sequence gates it immediately before the switch) */

      modifyreg32(CH32H41X_FLASH_ACTLR,
                  CH32H41X_FLASH_LATENCY_MASK, CH32H41X_FLASH_LATENCY_DIV2);

      modifyreg32(CH32H41X_RCC_PLLCFGR, 0, CH32H41X_RCC_SYSPLL_GATE);

      /* Switch the system clock source to the SYSPLL (bounded) */

      modifyreg32(CH32H41X_RCC_CFGR0, CH32H41X_RCC_SW_MASK,
                  CH32H41X_RCC_SW_PLL);

      timeout = HSE_STARTUP_TIMEOUT;
      while ((getreg32(CH32H41X_RCC_CFGR0) & CH32H41X_RCC_SWS_MASK) !=
             CH32H41X_RCC_SWS_PLL)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      /* Verify the final state: retry when the boot ROM interference
       * (or the hardware) undid part of the sequence */

      reg = getreg32(CH32H41X_RCC_PLLCFGR);
      if ((reg & CH32H41X_RCC_SYSPLL_GATE) != 0 &&
          (reg & CH32H41X_RCC_SYSPLL_SEL_MASK) == 0 &&
          (reg & (CH32H41X_RCC_PLLMUL_MASK | CH32H41X_RCC_PLLSRC |
                  CH32H41X_RCC_PLL_SRCDIV_MASK)) ==
          (CH32H41X_RCC_PLLMUL16 | CH32H41X_RCC_PLLSRC_HSE |
           CH32H41X_RCC_PLL_SRCDIV_DIV1) &&
          (getreg32(CH32H41X_RCC_CTLR) &
           (CH32H41X_RCC_HSEON | CH32H41X_RCC_HSERDY |
            CH32H41X_RCC_PLLON | CH32H41X_RCC_PLLRDY)) ==
          (CH32H41X_RCC_HSEON | CH32H41X_RCC_HSERDY |
           CH32H41X_RCC_PLLON | CH32H41X_RCC_PLLRDY) &&
          (getreg32(CH32H41X_RCC_CFGR0) & CH32H41X_RCC_SWS_MASK) ==
          CH32H41X_RCC_SWS_PLL)
        {
          g_ch32h41x_sysclk = 400000000ul;
          g_ch32h41x_hclk   = 100000000ul;
          return;
        }
    }

  /* All retries failed: keep running from whatever clock the boot ROM
   * left behind.  The frequency variables keep their reset defaults so
   * the driver baud math stays at the HSI-derived value. */
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
