/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_rcc.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_RCC_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_RCC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses (RCC block, 32-bit registers) */

#define CH32H41X_RCC_CTLR       (CH32H41X_RCC_BASE + 0x00)  /* clock control */
#define CH32H41X_RCC_CFGR0      (CH32H41X_RCC_BASE + 0x04)  /* clock configuration */
#define CH32H41X_RCC_PLLCFGR    (CH32H41X_RCC_BASE + 0x08)  /* SYSPLL configuration */
#define CH32H41X_RCC_INTR       (CH32H41X_RCC_BASE + 0x0c)  /* clock interrupt */
#define CH32H41X_RCC_HB2PRSTR   (CH32H41X_RCC_BASE + 0x10)  /* HB2 peripheral reset */
#define CH32H41X_RCC_HB1PRSTR   (CH32H41X_RCC_BASE + 0x14)  /* HB1 peripheral reset */
#define CH32H41X_RCC_HBPCENR    (CH32H41X_RCC_BASE + 0x18)  /* HB peripheral clock enable */
#define CH32H41X_RCC_HB2PCENR   (CH32H41X_RCC_BASE + 0x1c)  /* HB2 peripheral clock enable */
#define CH32H41X_RCC_HB1PCENR   (CH32H41X_RCC_BASE + 0x20)  /* HB1 peripheral clock enable */
#define CH32H41X_RCC_BDCTLR     (CH32H41X_RCC_BASE + 0x24)  /* backup domain control */
#define CH32H41X_RCC_RSTSCKR    (CH32H41X_RCC_BASE + 0x28)  /* reset status / clock control */
#define CH32H41X_RCC_HBRSTR     (CH32H41X_RCC_BASE + 0x2c)  /* HB peripheral reset */
#define CH32H41X_RCC_CFGR2      (CH32H41X_RCC_BASE + 0x30)  /* clock configuration 2 */
#define CH32H41X_RCC_PLLCFGR2   (CH32H41X_RCC_BASE + 0x34)  /* USBHS/SerDes PLL configuration */

/* CTLR bits */

#define CH32H41X_RCC_HSION      (1ul << 0)      /* HSI enable */
#define CH32H41X_RCC_HSIRDY     (1ul << 1)      /* HSI ready */
#define CH32H41X_RCC_HSEON      (1ul << 16)     /* HSE enable */
#define CH32H41X_RCC_HSERDY     (1ul << 17)     /* HSE ready */
#define CH32H41X_RCC_CSSON      (1ul << 19)     /* clock security system */
#define CH32H41X_RCC_USBHS_PLLON (1ul << 20)    /* USBHS PLL enable */
#define CH32H41X_RCC_USBHS_PLLRDY (1ul << 21)   /* USBHS PLL ready */
#define CH32H41X_RCC_PLLON      (1ul << 24)     /* SYSPLL enable */
#define CH32H41X_RCC_PLLRDY     (1ul << 25)     /* SYSPLL ready */

/* CFGR0 bits */

#define CH32H41X_RCC_SW_MASK    (3ul << 0)      /* system clock switch */
#define CH32H41X_RCC_SW_HSI     (0ul << 0)
#define CH32H41X_RCC_SW_HSE     (1ul << 0)
#define CH32H41X_RCC_SW_PLL     (2ul << 0)
#define CH32H41X_RCC_SWS_MASK   (3ul << 2)      /* system clock switch status */
#define CH32H41X_RCC_SWS_HSI    (0ul << 2)
#define CH32H41X_RCC_SWS_HSE    (1ul << 2)
#define CH32H41X_RCC_SWS_PLL    (2ul << 2)
#define CH32H41X_RCC_HPRE_SHIFT 4               /* V5F core / HB clock prescaler */
#define CH32H41X_RCC_HPRE_MASK  (0xful << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV1  (0x0ul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV2  (0x8ul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV4  (0x9ul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV8  (0xaul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV16 (0xbul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV32 (0xcul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV64 (0xdul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV128 (0xeul << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_HPRE_DIV256 (0xful << CH32H41X_RCC_HPRE_SHIFT)
#define CH32H41X_RCC_FPRE_SHIFT 16              /* V3F core clock prescaler */
#define CH32H41X_RCC_FPRE_MASK  (0x7ul << CH32H41X_RCC_FPRE_SHIFT)
#define CH32H41X_RCC_FPRE_DIV1  (0x0ul << CH32H41X_RCC_FPRE_SHIFT)
#define CH32H41X_RCC_FPRE_DIV2  (0x1ul << CH32H41X_RCC_FPRE_SHIFT)
#define CH32H41X_RCC_FPRE_DIV4  (0x2ul << CH32H41X_RCC_FPRE_SHIFT)

/* PLLCFGR bits */

#define CH32H41X_RCC_PLLMUL_SHIFT 0             /* PLL multiplication factor */
#define CH32H41X_RCC_PLLMUL_MASK  (0x1ful << CH32H41X_RCC_PLLMUL_SHIFT)
#define CH32H41X_RCC_PLLMUL(n)  (((n) - 1) << CH32H41X_RCC_PLLMUL_SHIFT)
#define CH32H41X_RCC_PLLMUL16   (15ul << CH32H41X_RCC_PLLMUL_SHIFT)
#define CH32H41X_RCC_PLLSRC     (1ul << 5)      /* PLL entry clock source */
#define CH32H41X_RCC_PLLSRC_HSI (0ul << 5)
#define CH32H41X_RCC_PLLSRC_HSE (1ul << 5)
#define CH32H41X_RCC_PLL_SRCDIV_SHIFT 8         /* PLL input divider */
#define CH32H41X_RCC_PLL_SRCDIV_MASK  (0xful << CH32H41X_RCC_PLL_SRCDIV_SHIFT)
#define CH32H41X_RCC_PLL_SRCDIV_DIV1  (0x0ul << CH32H41X_RCC_PLL_SRCDIV_SHIFT)
#define CH32H41X_RCC_SYSPLL_SEL_SHIFT 28        /* SYSPLL source select */
#define CH32H41X_RCC_SYSPLL_SEL_MASK  (0x7ul << CH32H41X_RCC_SYSPLL_SEL_SHIFT)
#define CH32H41X_RCC_SYSPLL_SEL_PLL   (0x0ul << CH32H41X_RCC_SYSPLL_SEL_SHIFT)
#define CH32H41X_RCC_SYSPLL_SEL_USBHS (0x4ul << CH32H41X_RCC_SYSPLL_SEL_SHIFT)
#define CH32H41X_RCC_SYSPLL_SEL_ETH   (0x5ul << CH32H41X_RCC_SYSPLL_SEL_SHIFT)
#define CH32H41X_RCC_SYSPLL_SEL_SERDES (0x6ul << CH32H41X_RCC_SYSPLL_SEL_SHIFT)
#define CH32H41X_RCC_SYSPLL_SEL_USBSS (0x7ul << CH32H41X_RCC_SYSPLL_SEL_SHIFT)
#define CH32H41X_RCC_SYSPLL_GATE (1ul << 31)    /* SYSPLL output gate */

/* PLLCFGR2 bits (USBHS PLL) */

#define CH32H41X_RCC_USBHSPLLSRC (1ul << 0)     /* USBHS PLL source: 0=HSE, 1=HSI */
#define CH32H41X_RCC_USBHSPLLSRC_HSE (0ul << 0)
#define CH32H41X_RCC_USBHSPLLSRC_HSI (1ul << 0)
#define CH32H41X_RCC_USBHSPLL_REFSEL_SHIFT 2    /* USBHS PLL reference frequency */
#define CH32H41X_RCC_USBHSPLL_REFSEL_MASK (0x3ul << CH32H41X_RCC_USBHSPLL_REFSEL_SHIFT)
#define CH32H41X_RCC_USBHSPLL_REFSEL_25MHZ (0x0ul << CH32H41X_RCC_USBHSPLL_REFSEL_SHIFT)
#define CH32H41X_RCC_USBHSPLL_REFSEL_20MHZ (0x1ul << CH32H41X_RCC_USBHSPLL_REFSEL_SHIFT)
#define CH32H41X_RCC_USBHSPLL_REFSEL_24MHZ (0x2ul << CH32H41X_RCC_USBHSPLL_REFSEL_SHIFT)
#define CH32H41X_RCC_USBHSPLL_REFSEL_32MHZ (0x3ul << CH32H41X_RCC_USBHSPLL_REFSEL_SHIFT)

/* INTR bits (interrupt flags / enables / clears), WCH F1-style layout */

#define CH32H41X_RCC_HSIRDYC    (1ul << 18)     /* HSI ready clear */
#define CH32H41X_RCC_HSERDYC    (1ul << 19)     /* HSE ready clear */
#define CH32H41X_RCC_PLLRDYC    (1ul << 20)     /* PLL ready clear */
#define CH32H41X_RCC_ALLFLAGS   0x00ff0000ul    /* clear all ready flags */

/* HB2 peripheral clock enable bits (RCC_HB2PCENR) */

#define CH32H41X_RCC_HB2_AFIO   (1ul << 0)
#define CH32H41X_RCC_HB2_GPIOA  (1ul << 2)
#define CH32H41X_RCC_HB2_GPIOB  (1ul << 3)
#define CH32H41X_RCC_HB2_GPIOC  (1ul << 4)
#define CH32H41X_RCC_HB2_GPIOD  (1ul << 5)
#define CH32H41X_RCC_HB2_GPIOE  (1ul << 6)
#define CH32H41X_RCC_HB2_GPIOF  (1ul << 7)
#define CH32H41X_RCC_HB2_USART1 (1ul << 14)

/* HB1 peripheral clock enable bits (RCC_HB1PCENR) */

#define CH32H41X_RCC_HB1_TIM2   (1ul << 0)
#define CH32H41X_RCC_HB1_TIM3   (1ul << 1)
#define CH32H41X_RCC_HB1_TIM4   (1ul << 2)
#define CH32H41X_RCC_HB1_TIM5   (1ul << 3)
#define CH32H41X_RCC_HB1_TIM6   (1ul << 4)
#define CH32H41X_RCC_HB1_TIM7   (1ul << 5)
#define CH32H41X_RCC_HB1_USART6 (1ul << 6)
#define CH32H41X_RCC_HB1_USART7 (1ul << 7)
#define CH32H41X_RCC_HB1_USART8 (1ul << 8)
#define CH32H41X_RCC_HB1_SDIO   (1ul << 11)
#define CH32H41X_RCC_HB1_USART2 (1ul << 17)
#define CH32H41X_RCC_HB1_USART3 (1ul << 18)
#define CH32H41X_RCC_HB1_USART4 (1ul << 19)
#define CH32H41X_RCC_HB1_USART5 (1ul << 20)

/* HB peripheral clock enable bits (RCC_HBPCENR) */

#define CH32H41X_RCC_HB_DMA1    (1ul << 0)
#define CH32H41X_RCC_HB_DMA2    (1ul << 1)
#define CH32H41X_RCC_HB_RNG     (1ul << 2)
#define CH32H41X_RCC_HB_SDMMC   (1ul << 3)
#define CH32H41X_RCC_HB_USBFS   (1ul << 5)

/* HB2 peripheral reset bits (RCC_HB2PRSTR) - same bit order as HB2PCENR */

/* Flash latency (flash control ACTLR register) */

#define CH32H41X_FLASH_ACTLR    (CH32H41X_FLASH_RBASE + 0x00)
#define CH32H41X_FLASH_LATENCY_MASK 0x3ul
#define CH32H41X_FLASH_LATENCY_DIV1 0x0ul     /* HCLK/1 */
#define CH32H41X_FLASH_LATENCY_DIV2 0x1ul     /* HCLK/2 */
#define CH32H41X_FLASH_LATENCY_DIV4 0x2ul     /* HCLK/4 */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_RCC_H */
