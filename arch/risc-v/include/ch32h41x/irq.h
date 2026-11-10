/****************************************************************************
 * arch/risc-v/include/ch32h41x/irq.h
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

#ifndef __ARCH_RISCV_INCLUDE_CH32H41X_IRQ_H
#define __ARCH_RISCV_INCLUDE_CH32H41X_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <arch/irq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The CH32H41x PFIC delivers all interrupts with mcause values of
 * 0x80000000 | n where n is the vector index shown below.  NuttX irq
 * numbers are offset by RISCV_IRQ_ASYNC (== 16) so that they do not
 * collide with the synchronous exception numbers 0..15.
 *
 * Vector indices 0..31 are core-local interrupts (PFIC managed); vector
 * indices 32..148 are peripheral interrupts.
 */

#define CH32H41X_IRQ_FIRST        RISCV_IRQ_ASYNC

/* Core interrupts */

#define CH32H41X_IRQ_NMI          (CH32H41X_IRQ_FIRST + 2)   /* NMI */
#define CH32H41X_IRQ_HARDFAULT    (CH32H41X_IRQ_FIRST + 3)   /* Hard fault */
#define CH32H41X_IRQ_SYSTICK0     (CH32H41X_IRQ_FIRST + 12)  /* System timer 0 (V3F) */
#define CH32H41X_IRQ_SYSTICK1     (CH32H41X_IRQ_FIRST + 13)  /* System timer 1 (V5F) */
#define CH32H41X_IRQ_SWINT        (CH32H41X_IRQ_FIRST + 14)  /* Software interrupt */
#define CH32H41X_IRQ_IPC0         (CH32H41X_IRQ_FIRST + 16)  /* Inter-processor channel 0 */
#define CH32H41X_IRQ_IPC1         (CH32H41X_IRQ_FIRST + 17)  /* Inter-processor channel 1 */
#define CH32H41X_IRQ_IPC2         (CH32H41X_IRQ_FIRST + 18)  /* Inter-processor channel 2 */
#define CH32H41X_IRQ_IPC3         (CH32H41X_IRQ_FIRST + 19)  /* Inter-processor channel 3 */
#define CH32H41X_IRQ_HSEM         (CH32H41X_IRQ_FIRST + 28)  /* Hardware semaphore */

/* Peripheral interrupts */

#define CH32H41X_IRQ_WWDG         (CH32H41X_IRQ_FIRST + 32)  /* Window watchdog */
#define CH32H41X_IRQ_EXTI15_8     (CH32H41X_IRQ_FIRST + 33)  /* External lines [15:8] */
#define CH32H41X_IRQ_FLASH        (CH32H41X_IRQ_FIRST + 34)  /* Flash global */
#define CH32H41X_IRQ_RCC          (CH32H41X_IRQ_FIRST + 35)  /* RCC global */
#define CH32H41X_IRQ_EXTI7_0      (CH32H41X_IRQ_FIRST + 36)  /* External lines [7:0] */
#define CH32H41X_IRQ_SPI1         (CH32H41X_IRQ_FIRST + 37)  /* SPI1 */
#define CH32H41X_IRQ_DMA1CH2      (CH32H41X_IRQ_FIRST + 38)  /* DMA1 channel 2 */
#define CH32H41X_IRQ_DMA1CH3      (CH32H41X_IRQ_FIRST + 39)  /* DMA1 channel 3 */
#define CH32H41X_IRQ_DMA1CH4      (CH32H41X_IRQ_FIRST + 40)  /* DMA1 channel 4 */
#define CH32H41X_IRQ_DMA1CH5      (CH32H41X_IRQ_FIRST + 41)  /* DMA1 channel 5 */
#define CH32H41X_IRQ_DMA1CH6      (CH32H41X_IRQ_FIRST + 42)  /* DMA1 channel 6 */
#define CH32H41X_IRQ_DMA1CH7      (CH32H41X_IRQ_FIRST + 43)  /* DMA1 channel 7 */
#define CH32H41X_IRQ_DMA1CH8      (CH32H41X_IRQ_FIRST + 44)  /* DMA1 channel 8 */
#define CH32H41X_IRQ_USART2       (CH32H41X_IRQ_FIRST + 45)  /* USART2 */
#define CH32H41X_IRQ_I2C1EV       (CH32H41X_IRQ_FIRST + 46)  /* I2C1 event */
#define CH32H41X_IRQ_I2C1ER       (CH32H41X_IRQ_FIRST + 47)  /* I2C1 error */
#define CH32H41X_IRQ_USART1       (CH32H41X_IRQ_FIRST + 48)  /* USART1 */
#define CH32H41X_IRQ_SPI2         (CH32H41X_IRQ_FIRST + 49)  /* SPI2 */
#define CH32H41X_IRQ_SPI3         (CH32H41X_IRQ_FIRST + 50)  /* SPI3 */
#define CH32H41X_IRQ_SPI4         (CH32H41X_IRQ_FIRST + 51)  /* SPI4 */
#define CH32H41X_IRQ_I2C2EV       (CH32H41X_IRQ_FIRST + 52)  /* I2C2 event */
#define CH32H41X_IRQ_I2C2ER       (CH32H41X_IRQ_FIRST + 53)  /* I2C2 error */
#define CH32H41X_IRQ_USBPD        (CH32H41X_IRQ_FIRST + 54)  /* USB PD */
#define CH32H41X_IRQ_USBPDWKUP    (CH32H41X_IRQ_FIRST + 55)  /* USB PD wakeup */
#define CH32H41X_IRQ_USBHS        (CH32H41X_IRQ_FIRST + 56)  /* USBHS */
#define CH32H41X_IRQ_DMA1CH1      (CH32H41X_IRQ_FIRST + 57)  /* DMA1 channel 1 */
#define CH32H41X_IRQ_CAN1SCE      (CH32H41X_IRQ_FIRST + 58)  /* CAN1 SCE */
#define CH32H41X_IRQ_CAN1TX       (CH32H41X_IRQ_FIRST + 59)  /* CAN1 TX */
#define CH32H41X_IRQ_CAN1RX0      (CH32H41X_IRQ_FIRST + 60)  /* CAN1 RX0 */
#define CH32H41X_IRQ_CAN1RX1      (CH32H41X_IRQ_FIRST + 61)  /* CAN1 RX1 */
#define CH32H41X_IRQ_USBSS        (CH32H41X_IRQ_FIRST + 62)  /* USBSS */
#define CH32H41X_IRQ_USBSSLINK    (CH32H41X_IRQ_FIRST + 63)  /* USBSS link */
#define CH32H41X_IRQ_USBHSWKUP    (CH32H41X_IRQ_FIRST + 64)  /* USBHS wakeup */
#define CH32H41X_IRQ_USBSSWKUP    (CH32H41X_IRQ_FIRST + 65)  /* USBSS wakeup */
#define CH32H41X_IRQ_RTCALARM     (CH32H41X_IRQ_FIRST + 66)  /* RTC alarm through EXTI */
#define CH32H41X_IRQ_USBFS        (CH32H41X_IRQ_FIRST + 67)  /* USBFS */
#define CH32H41X_IRQ_USBFSWKUP    (CH32H41X_IRQ_FIRST + 68)  /* USBFS wakeup */
#define CH32H41X_IRQ_ADC12        (CH32H41X_IRQ_FIRST + 69)  /* ADC1 and ADC2 */
#define CH32H41X_IRQ_TIM1BRK      (CH32H41X_IRQ_FIRST + 70)  /* TIM1 break */
#define CH32H41X_IRQ_TIM1UP       (CH32H41X_IRQ_FIRST + 71)  /* TIM1 update */
#define CH32H41X_IRQ_TIM1TRGCOM   (CH32H41X_IRQ_FIRST + 72)  /* TIM1 trigger and commutation */
#define CH32H41X_IRQ_TIM1CC       (CH32H41X_IRQ_FIRST + 73)  /* TIM1 capture compare */
#define CH32H41X_IRQ_TIM2         (CH32H41X_IRQ_FIRST + 74)  /* TIM2 */
#define CH32H41X_IRQ_TIM3         (CH32H41X_IRQ_FIRST + 75)  /* TIM3 */
#define CH32H41X_IRQ_TIM4         (CH32H41X_IRQ_FIRST + 76)  /* TIM4 */
#define CH32H41X_IRQ_TIM5         (CH32H41X_IRQ_FIRST + 77)  /* TIM5 */
#define CH32H41X_IRQ_I2C3EV       (CH32H41X_IRQ_FIRST + 78)  /* I2C3 event */
#define CH32H41X_IRQ_I2C3ER       (CH32H41X_IRQ_FIRST + 79)  /* I2C3 error */
#define CH32H41X_IRQ_I2C4EV       (CH32H41X_IRQ_FIRST + 80)  /* I2C4 event */
#define CH32H41X_IRQ_I2C4ER       (CH32H41X_IRQ_FIRST + 81)  /* I2C4 error */
#define CH32H41X_IRQ_QSPI1        (CH32H41X_IRQ_FIRST + 82)  /* QSPI1 */
#define CH32H41X_IRQ_SERDES       (CH32H41X_IRQ_FIRST + 83)  /* SerDes */
#define CH32H41X_IRQ_USART3       (CH32H41X_IRQ_FIRST + 84)  /* USART3 */
#define CH32H41X_IRQ_USART4       (CH32H41X_IRQ_FIRST + 85)  /* USART4 */
#define CH32H41X_IRQ_TIM8BRK      (CH32H41X_IRQ_FIRST + 86)  /* TIM8 break */
#define CH32H41X_IRQ_TIM8UP       (CH32H41X_IRQ_FIRST + 87)  /* TIM8 update */
#define CH32H41X_IRQ_TIM8TRGCOM   (CH32H41X_IRQ_FIRST + 88)  /* TIM8 trigger and commutation */
#define CH32H41X_IRQ_TIM8CC       (CH32H41X_IRQ_FIRST + 89)  /* TIM8 capture compare */
#define CH32H41X_IRQ_TIM9         (CH32H41X_IRQ_FIRST + 90)  /* TIM9 */
#define CH32H41X_IRQ_TIM10        (CH32H41X_IRQ_FIRST + 91)  /* TIM10 */
#define CH32H41X_IRQ_TIM11        (CH32H41X_IRQ_FIRST + 92)  /* TIM11 */
#define CH32H41X_IRQ_TIM12        (CH32H41X_IRQ_FIRST + 93)  /* TIM12 */
#define CH32H41X_IRQ_FMC          (CH32H41X_IRQ_FIRST + 94)  /* FMC */
#define CH32H41X_IRQ_SDMMC        (CH32H41X_IRQ_FIRST + 95)  /* SDMMC */
#define CH32H41X_IRQ_LPTIM1       (CH32H41X_IRQ_FIRST + 96)  /* LPTIM1 */
#define CH32H41X_IRQ_LPTIM2       (CH32H41X_IRQ_FIRST + 97)  /* LPTIM2 */
#define CH32H41X_IRQ_USART5       (CH32H41X_IRQ_FIRST + 98)  /* USART5 */
#define CH32H41X_IRQ_USART6       (CH32H41X_IRQ_FIRST + 99)  /* USART6 */
#define CH32H41X_IRQ_TIM6         (CH32H41X_IRQ_FIRST + 100) /* TIM6 */
#define CH32H41X_IRQ_TIM7         (CH32H41X_IRQ_FIRST + 101) /* TIM7 */
#define CH32H41X_IRQ_DMA2CH1      (CH32H41X_IRQ_FIRST + 102) /* DMA2 channel 1 */
#define CH32H41X_IRQ_DMA2CH2      (CH32H41X_IRQ_FIRST + 103) /* DMA2 channel 2 */
#define CH32H41X_IRQ_DMA2CH3      (CH32H41X_IRQ_FIRST + 104) /* DMA2 channel 3 */
#define CH32H41X_IRQ_DMA2CH4      (CH32H41X_IRQ_FIRST + 105) /* DMA2 channel 4 */
#define CH32H41X_IRQ_DMA2CH5      (CH32H41X_IRQ_FIRST + 106) /* DMA2 channel 5 */
#define CH32H41X_IRQ_DMA2CH6      (CH32H41X_IRQ_FIRST + 107) /* DMA2 channel 6 */
#define CH32H41X_IRQ_DMA2CH7      (CH32H41X_IRQ_FIRST + 108) /* DMA2 channel 7 */
#define CH32H41X_IRQ_DMA2CH8      (CH32H41X_IRQ_FIRST + 109) /* DMA2 channel 8 */
#define CH32H41X_IRQ_ETH          (CH32H41X_IRQ_FIRST + 110) /* Ethernet */
#define CH32H41X_IRQ_ETHWKUP      (CH32H41X_IRQ_FIRST + 111) /* Ethernet wakeup */
#define CH32H41X_IRQ_CAN2SCE      (CH32H41X_IRQ_FIRST + 112) /* CAN2 SCE */
#define CH32H41X_IRQ_CAN2TX       (CH32H41X_IRQ_FIRST + 113) /* CAN2 TX */
#define CH32H41X_IRQ_CAN2RX0      (CH32H41X_IRQ_FIRST + 114) /* CAN2 RX0 */
#define CH32H41X_IRQ_CAN2RX1      (CH32H41X_IRQ_FIRST + 115) /* CAN2 RX1 */
#define CH32H41X_IRQ_USART7       (CH32H41X_IRQ_FIRST + 116) /* USART7 */
#define CH32H41X_IRQ_USART8       (CH32H41X_IRQ_FIRST + 117) /* USART8 */
#define CH32H41X_IRQ_I3CEV        (CH32H41X_IRQ_FIRST + 118) /* I3C event */
#define CH32H41X_IRQ_I3CER        (CH32H41X_IRQ_FIRST + 119) /* I3C error */
#define CH32H41X_IRQ_DVP          (CH32H41X_IRQ_FIRST + 120) /* DVP */
#define CH32H41X_IRQ_ECDC         (CH32H41X_IRQ_FIRST + 121) /* ECDC crypto */
#define CH32H41X_IRQ_PIOC         (CH32H41X_IRQ_FIRST + 122) /* PIOC */
#define CH32H41X_IRQ_SAI          (CH32H41X_IRQ_FIRST + 123) /* SAI */
#define CH32H41X_IRQ_LTDC         (CH32H41X_IRQ_FIRST + 124) /* LTDC */
#define CH32H41X_IRQ_GPHA         (CH32H41X_IRQ_FIRST + 125) /* GPHA */
#define CH32H41X_IRQ_DFSDM0       (CH32H41X_IRQ_FIRST + 127) /* DFSDM0 */
#define CH32H41X_IRQ_DFSDM1       (CH32H41X_IRQ_FIRST + 128) /* DFSDM1 */
#define CH32H41X_IRQ_SWPMI        (CH32H41X_IRQ_FIRST + 131) /* SWPMI */
#define CH32H41X_IRQ_QSPI2        (CH32H41X_IRQ_FIRST + 134) /* QSPI2 */
#define CH32H41X_IRQ_SWPMIWKUP    (CH32H41X_IRQ_FIRST + 135) /* SWPMI wakeup */
#define CH32H41X_IRQ_CAN3SCE      (CH32H41X_IRQ_FIRST + 136) /* CAN3 SCE */
#define CH32H41X_IRQ_CAN3TX       (CH32H41X_IRQ_FIRST + 137) /* CAN3 TX */
#define CH32H41X_IRQ_CAN3RX0      (CH32H41X_IRQ_FIRST + 138) /* CAN3 RX0 */
#define CH32H41X_IRQ_CAN3RX1      (CH32H41X_IRQ_FIRST + 139) /* CAN3 RX1 */
#define CH32H41X_IRQ_LPTIM2WKUP   (CH32H41X_IRQ_FIRST + 140) /* LPTIM2 wakeup */
#define CH32H41X_IRQ_LPTIM1WKUP   (CH32H41X_IRQ_FIRST + 141) /* LPTIM1 wakeup */
#define CH32H41X_IRQ_I3CWKUP      (CH32H41X_IRQ_FIRST + 142) /* I3C wakeup */
#define CH32H41X_IRQ_RTC          (CH32H41X_IRQ_FIRST + 143) /* RTC global */
#define CH32H41X_IRQ_HSADC        (CH32H41X_IRQ_FIRST + 144) /* HSADC */
#define CH32H41X_IRQ_UHSIF        (CH32H41X_IRQ_FIRST + 145) /* UHSIF */
#define CH32H41X_IRQ_RNG          (CH32H41X_IRQ_FIRST + 146) /* RNG */
#define CH32H41X_IRQ_SDIO         (CH32H41X_IRQ_FIRST + 147) /* SDIO */
#define CH32H41X_IRQ_USARTWKUP    (CH32H41X_IRQ_FIRST + 148) /* USART wakeup */

/* Total number of IRQ numbers: exceptions 0..15 plus the asynchronous
 * interrupts above (last vector index is 148).
 */

#define NR_IRQS                   (CH32H41X_IRQ_USARTWKUP + 1)

#endif /* __ARCH_RISCV_INCLUDE_CH32H41X_IRQ_H */
