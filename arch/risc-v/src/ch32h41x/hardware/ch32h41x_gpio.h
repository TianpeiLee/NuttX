/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_gpio.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_GPIO_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_GPIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GPIO register offsets (STM32-F1 style) */

#define CH32H41X_GPIO_CFGLR     0x00  /* port config register low (pins 0-7) */
#define CH32H41X_GPIO_CFGHR     0x04  /* port config register high (pins 8-15) */
#define CH32H41X_GPIO_INDR      0x08  /* port input data */
#define CH32H41X_GPIO_OUTDR     0x0c  /* port output data */
#define CH32H41X_GPIO_BSHR      0x10  /* port bit set/reset */
#define CH32H41X_GPIO_BCR       0x14  /* port bit reset */
#define CH32H41X_GPIO_LCKR      0x18  /* port configuration lock */
#define CH32H41X_GPIO_SPEED     0x1c  /* port drive speed */

/* CFGLR/CFGHR pin nibble (MODE[1:0] | CNF[1:0] per pin) */

#define CH32H41X_GPIO_CFGLR_REG(pin) (CH32H41X_GPIO_CFGLR + (((pin) >> 3) << 2))

#define CH32H41X_GPIO_MODE_SHIFT(pin) (((pin) & 7) * 4)
#define CH32H41X_GPIO_MODE_MASK(pin)  (0xful << CH32H41X_GPIO_MODE_SHIFT(pin))

#define CH32H41X_GPIO_MODE_IN_FLOAT  0x4  /* input, floating (CNF=01, MODE=00) */
#define CH32H41X_GPIO_MODE_IN_PULL   0x8  /* input, pull-up/down (CNF=10, MODE=00) */
#define CH32H41X_GPIO_MODE_IN_ANALOG 0x0  /* input, analog (CNF=00, MODE=00) */
#define CH32H41X_GPIO_MODE_OUT_PP_10 0x2  /* output push-pull, 10MHz */
#define CH32H41X_GPIO_MODE_OUT_PP_50 0x3  /* output push-pull, 50MHz */
#define CH32H41X_GPIO_MODE_OUT_OD_50 0x7  /* output open-drain, 50MHz */
#define CH32H41X_GPIO_MODE_AF_PP_50  0xb  /* alternate function push-pull, 50MHz */
#define CH32H41X_GPIO_MODE_AF_OD_50  0xf  /* alternate function open-drain, 50MHz */

/* BSHR bits */

#define CH32H41X_GPIO_BSHR_SET(n)   (1ul << (n))
#define CH32H41X_GPIO_BSHR_RESET(n) (1ul << ((n) + 16))

/* AFIO register offsets */

#define CH32H41X_AFIO_PCFR1     0x00  /* remap configuration */

/* AFIO per-port alternate function select registers, 4 bits per pin.
 * Low register covers pins 0..7, high register covers pins 8..15.
 */

#define CH32H41X_AFIO_AF(port)  (CH32H41X_AFIO_BASE + 0x04 + (port) * 8)
#define CH32H41X_AFIO_AFLR(port) (CH32H41X_AFIO_AF(port))
#define CH32H41X_AFIO_AFHR(port) (CH32H41X_AFIO_AF(port) + 4)

#define CH32H41X_AFIO_AF_SHIFT(pin) (((pin) & 7) * 4)
#define CH32H41X_AFIO_AF_MASK(pin)  (0xful << CH32H41X_AFIO_AF_SHIFT(pin))

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_GPIO_H */
