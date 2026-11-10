/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_exti.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_EXTI_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_EXTI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses */

#define CH32H41X_EXTI_INTENR     (CH32H41X_EXTI_BASE + 0x00)  /* interrupt mask */
#define CH32H41X_EXTI_EVENR      (CH32H41X_EXTI_BASE + 0x04)  /* event mask */
#define CH32H41X_EXTI_RTENR      (CH32H41X_EXTI_BASE + 0x08)  /* rising trigger */
#define CH32H41X_EXTI_FTENR      (CH32H41X_EXTI_BASE + 0x0c)  /* falling trigger */
#define CH32H41X_EXTI_SWIEVR     (CH32H41X_EXTI_BASE + 0x10)  /* software interrupt */
#define CH32H41X_EXTI_INTFR      (CH32H41X_EXTI_BASE + 0x14)  /* pending (write 1 clear) */

/* AFIO port selection.  Each EXTICR register covers 8 lines with one
 * 4-bit nibble per line selecting the GPIO port index (0=A..5=F). */

#define CH32H41X_AFIO_EXTICR1    (CH32H41X_AFIO_BASE + 0x3c)  /* lines 0-7 */
#define CH32H41X_AFIO_EXTICR2    (CH32H41X_AFIO_BASE + 0x40)  /* lines 8-15 */

#define CH32H41X_AFIO_EXTICR_REG(pin) \
  ((pin) < 8 ? CH32H41X_AFIO_EXTICR1 : CH32H41X_AFIO_EXTICR2)
#define CH32H41X_AFIO_EXTICR_SHIFT(pin) (((pin) & 7) * 4)
#define CH32H41X_AFIO_EXTICR_MASK(pin)  (0xful << CH32H41X_AFIO_EXTICR_SHIFT(pin))

#define CH32H41X_EXTI_LINE(pin)  (1ul << (pin))

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_EXTI_H */
