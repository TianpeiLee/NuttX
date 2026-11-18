/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_memorymap.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_MEMORYMAP_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_MEMORYMAP_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Memory regions */

#define CH32H41X_FLASH_BASE     0x00000000  /* Code flash window (zero-wait) */
#define CH32H41X_FLASH_PGM      0x08000000  /* Flash programming alias */
#define CH32H41X_ITCM_BASE      0x200a0000  /* ITCM 256KiB (V5F) */
#define CH32H41X_DTCM_BASE      0x200e0000  /* DTCM 128KiB (V5F); 512B hw stack at base */
#define CH32H41X_SRAM_BASE      0x20100000  /* Shared SRAM 512KiB */

#define CH32H41X_ITCM_SIZE      (256 * 1024)
#define CH32H41X_DTCM_SIZE      (128 * 1024)
#define CH32H41X_SRAM_SIZE      (512 * 1024)
#define CH32H41X_CODEFLASH_SIZE (960 * 1024)  /* Code flash, mode 1 (DBMODE=1) */

/* Core private peripherals */

#define CH32H41X_HSEM_BASE      0xe000c000  /* Hardware semaphore */
#define CH32H41X_IPC_BASE       0xe000d000  /* Inter-processor communication */
#define CH32H41X_PFIC_BASE      0xe000e000  /* PFIC (interrupt controller) */
#define CH32H41X_SYSTICK0_BASE  0xe000f000  /* System timer 0 (V3F) */
#define CH32H41X_SYSTICK1_BASE  0xe000f080  /* System timer 1 (V5F) */

/* High performance bus peripherals (HBPERIPH_BASE = 0x40000000) */

#define CH32H41X_TIM2_BASE      0x40000000
#define CH32H41X_TIM3_BASE      0x40000400
#define CH32H41X_TIM4_BASE      0x40000800
#define CH32H41X_TIM5_BASE      0x40000c00
#define CH32H41X_TIM6_BASE      0x40001000
#define CH32H41X_TIM7_BASE      0x40001400
#define CH32H41X_USART6_BASE    0x40001800
#define CH32H41X_USART7_BASE    0x40001c00
#define CH32H41X_USART8_BASE    0x40002000
#define CH32H41X_LPTIM1_BASE    0x40002400
#define CH32H41X_RTC_BASE       0x40002800
#define CH32H41X_WWDG_BASE      0x40002c00
#define CH32H41X_IWDG_BASE      0x40003000
#define CH32H41X_LPTIM2_BASE    0x40003400
#define CH32H41X_SPI2_BASE      0x40003800
#define CH32H41X_SPI3_BASE      0x40003c00
#define CH32H41X_SPI4_BASE      0x40004000
#define CH32H41X_USART2_BASE    0x40004400
#define CH32H41X_USART3_BASE    0x40004800
#define CH32H41X_USART4_BASE    0x40004c00
#define CH32H41X_USART5_BASE    0x40005000
#define CH32H41X_I2C1_BASE      0x40005400
#define CH32H41X_I2C2_BASE      0x40005800
#define CH32H41X_I2C3_BASE      0x40005c00
#define CH32H41X_CAN1_BASE      0x40006400
#define CH32H41X_CAN2_BASE      0x40006800
#define CH32H41X_PWR_BASE       0x40007000
#define CH32H41X_DAC_BASE       0x40007400
#define CH32H41X_CAN3_BASE      0x40007800
#define CH32H41X_AFIO_BASE      0x40010000  /* Alternate function I/O */
#define CH32H41X_EXTI_BASE      0x40010400  /* External interrupt/event */
#define CH32H41X_GPIOA_BASE     0x40010800
#define CH32H41X_GPIOB_BASE     0x40010c00
#define CH32H41X_GPIOC_BASE     0x40011000
#define CH32H41X_GPIOD_BASE     0x40011400
#define CH32H41X_GPIOE_BASE     0x40011800
#define CH32H41X_GPIOF_BASE     0x40011c00
#define CH32H41X_ADC1_BASE      0x40012400
#define CH32H41X_ADC2_BASE      0x40012800
#define CH32H41X_TIM1_BASE      0x40012c00
#define CH32H41X_SPI1_BASE      0x40013000
#define CH32H41X_TIM8_BASE      0x40013400
#define CH32H41X_USART1_BASE    0x40013800
#define CH32H41X_TIM12_BASE     0x40013c00
#define CH32H41X_I2C4_BASE      0x40014000
#define CH32H41X_TIM9_BASE      0x40014c00
#define CH32H41X_TIM10_BASE     0x40015000
#define CH32H41X_TIM11_BASE     0x40015400
#define CH32H41X_SDIO_BASE      0x40018000  /* SDIO controller */
#define CH32H41X_DMA1_BASE      0x40020000
#define CH32H41X_DMA2_BASE      0x40020400
#define CH32H41X_DMAMUX_BASE    0x40020800
#define CH32H41X_RCC_BASE       0x40021000
#define CH32H41X_FLASH_RBASE    0x40022000  /* Flash control registers */
#define CH32H41X_CRC_BASE       0x40023000
#define CH32H41X_USBFS_BASE     0x40023400
#define CH32H41X_RNG_BASE       0x40023c00
#define CH32H41X_SDMMC_BASE     0x40024000  /* 8-bit eMMC controller */
#define CH32H41X_USBPD_BASE     0x40024400
#define CH32H41X_QSPI1_BASE     0x40024c00
#define CH32H41X_QSPI2_BASE     0x40025000
#define CH32H41X_FMC_RBASE      0x40025400

/* GPIO port count */

#define CH32H41X_GPIO_NPORTS    6           /* GPIOA..GPIOF */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_MEMORYMAP_H */
