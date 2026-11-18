/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/boot/boot_clock.c
 *
 * Standalone 400MHz clock bring-up for the V3F boot loader: the same
 * EVT-derived sequence (SYSCLK_400M_CoreCLK_V5F_400M_V3F_100M_HSE) as
 * ch32h41x_clockconfig.c, including the bounded waits, the mux
 * synchronization polls and the verify-and-retry that defeats the V5F
 * boot-ROM RCC race during cold power-on.  Register accesses use local
 * helpers so this file has no NuttX dependencies.
 ****************************************************************************/

#include <stdint.h>

#include "hardware/ch32h41x_rcc.h"

#define BOOT_HSE_STARTUP_TIMEOUT 0x100000ul

static uint32_t boot_sysclk = 25000000ul;
static uint32_t boot_hclk   = 25000000ul;

static inline void boot_putreg32(uintptr_t addr, uint32_t value)
{
  *(volatile uint32_t *)addr = value;
}

static inline uint32_t boot_getreg32(uintptr_t addr)
{
  return *(volatile uint32_t *)addr;
}

static inline void boot_modifyreg32(uintptr_t addr, uint32_t clrbits,
                                    uint32_t setbits)
{
  boot_putreg32(addr, (boot_getreg32(addr) & ~clrbits) | setbits);
}

uint32_t boot_hclk_frequency(void)
{
  return boot_hclk;
}

void boot_setclock_400m(void)
{
  uint32_t timeout;
  uint32_t reg;
  unsigned retry;

  for (retry = 0; retry < 4; retry++)
    {
      /* EVT SystemInit prologue: known analog state */

      boot_modifyreg32(CH32H41X_RCC_CTLR, 0x95590000ul, 0);
      boot_modifyreg32(CH32H41X_RCC_CTLR, CH32H41X_RCC_HSEBYP, 0);
      boot_modifyreg32(CH32H41X_RCC_PLLCFGR, CH32H41X_RCC_SYSPLL_GATE, 0);

      /* HSE on and wait for ready */

      boot_modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_HSEON);

      timeout = BOOT_HSE_STARTUP_TIMEOUT;
      while ((boot_getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_HSERDY) == 0)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      if ((boot_getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_HSERDY) == 0)
        {
          continue;
        }

      /* SYSPLL = HSE * 16 = 400MHz */

      boot_modifyreg32(CH32H41X_RCC_PLLCFGR,
                       CH32H41X_RCC_PLLMUL_MASK |
                       CH32H41X_RCC_PLL_SRCDIV_MASK |
                       CH32H41X_RCC_PLLSRC,
                       CH32H41X_RCC_PLLMUL16 |
                       CH32H41X_RCC_PLL_SRCDIV_DIV1 |
                       CH32H41X_RCC_PLLSRC_HSE);

      timeout = BOOT_HSE_STARTUP_TIMEOUT;
      while ((boot_getreg32(CH32H41X_RCC_PLLCFGR) & CH32H41X_RCC_PLLSRC) !=
             CH32H41X_RCC_PLLSRC_HSE)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      /* PLL on and wait for lock */

      boot_modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_PLLON);

      timeout = BOOT_HSE_STARTUP_TIMEOUT;
      while ((boot_getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_PLLRDY) == 0)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      if ((boot_getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_PLLRDY) == 0)
        {
          continue;
        }

      /* Deselect and ungate the SYSPLL output */

      boot_modifyreg32(CH32H41X_RCC_PLLCFGR,
                       CH32H41X_RCC_SYSPLL_SEL_MASK | CH32H41X_RCC_SYSPLL_GATE,
                       0);

      timeout = BOOT_HSE_STARTUP_TIMEOUT;
      while ((boot_getreg32(CH32H41X_RCC_PLLCFGR) &
              CH32H41X_RCC_SYSPLL_SEL_MASK) != 0)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      /* Prescalers: V5F = SYSCLK/1, HCLK = SYSCLK/4 */

      boot_modifyreg32(CH32H41X_RCC_CFGR0,
                       CH32H41X_RCC_HPRE_MASK | CH32H41X_RCC_FPRE_MASK,
                       CH32H41X_RCC_HPRE_DIV1 | CH32H41X_RCC_FPRE_DIV4);

      /* Flash latency, then gate the SYSPLL output in */

      boot_modifyreg32(CH32H41X_FLASH_ACTLR,
                       CH32H41X_FLASH_LATENCY_MASK,
                       CH32H41X_FLASH_LATENCY_DIV2);

      boot_modifyreg32(CH32H41X_RCC_PLLCFGR, 0, CH32H41X_RCC_SYSPLL_GATE);

      /* Switch the system clock to the SYSPLL */

      boot_modifyreg32(CH32H41X_RCC_CFGR0, CH32H41X_RCC_SW_MASK,
                       CH32H41X_RCC_SW_PLL);

      timeout = BOOT_HSE_STARTUP_TIMEOUT;
      while ((boot_getreg32(CH32H41X_RCC_CFGR0) & CH32H41X_RCC_SWS_MASK) !=
             CH32H41X_RCC_SWS_PLL)
        {
          if (--timeout == 0)
            {
              break;
            }
        }

      /* Verify the final state and retry on interference */

      reg = boot_getreg32(CH32H41X_RCC_PLLCFGR);
      if ((reg & CH32H41X_RCC_SYSPLL_GATE) != 0 &&
          (reg & CH32H41X_RCC_SYSPLL_SEL_MASK) == 0 &&
          (reg & (CH32H41X_RCC_PLLMUL_MASK | CH32H41X_RCC_PLLSRC |
                  CH32H41X_RCC_PLL_SRCDIV_MASK)) ==
          (CH32H41X_RCC_PLLMUL16 | CH32H41X_RCC_PLLSRC_HSE |
           CH32H41X_RCC_PLL_SRCDIV_DIV1) &&
          (boot_getreg32(CH32H41X_RCC_CTLR) &
           (CH32H41X_RCC_HSEON | CH32H41X_RCC_HSERDY |
            CH32H41X_RCC_PLLON | CH32H41X_RCC_PLLRDY)) ==
          (CH32H41X_RCC_HSEON | CH32H41X_RCC_HSERDY |
           CH32H41X_RCC_PLLON | CH32H41X_RCC_PLLRDY) &&
          (boot_getreg32(CH32H41X_RCC_CFGR0) & CH32H41X_RCC_SWS_MASK) ==
          CH32H41X_RCC_SWS_PLL)
        {
          boot_sysclk = 400000000ul;
          boot_hclk   = 100000000ul;
          return;
        }
    }

  /* All retries failed: run from whatever the boot ROM left behind */
}
