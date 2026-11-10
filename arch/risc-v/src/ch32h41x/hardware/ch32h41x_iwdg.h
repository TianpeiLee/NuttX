/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_iwdg.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_IWDG_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_IWDG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses */

#define CH32H41X_IWDG_CTLR       (CH32H41X_IWDG_BASE + 0x00)
#define CH32H41X_IWDG_PSCR       (CH32H41X_IWDG_BASE + 0x04)
#define CH32H41X_IWDG_RLDR       (CH32H41X_IWDG_BASE + 0x08)
#define CH32H41X_IWDG_STATR      (CH32H41X_IWDG_BASE + 0x0c)

/* CTLR keys */

#define CH32H41X_IWDG_KEY_START  0xcccc         /* start the watchdog */
#define CH32H41X_IWDG_KEY_WRITE  0x5555         /* enable write access */
#define CH32H41X_IWDG_KEY_RELOAD 0xaaaa         /* reload the counter */

/* PSCR prescaler select (LSB 3 bits): clock = LSI/4/2^n */

#define CH32H41X_IWDG_PSC_SHIFT  0
#define CH32H41X_IWDG_PSC_MASK   (7ul << CH32H41X_IWDG_PSC_SHIFT)

/* STATR bits */

#define CH32H41X_IWDG_SR_PVU     (1ul << 0)     /* prescaler value update */
#define CH32H41X_IWDG_SR_RVU     (1ul << 1)     /* reload value update */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_IWDG_H */
