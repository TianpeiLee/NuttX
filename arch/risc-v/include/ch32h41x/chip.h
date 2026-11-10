/****************************************************************************
 * arch/risc-v/include/ch32h41x/chip.h
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

#ifndef __ARCH_RISCV_INCLUDE_CH32H41X_CHIP_H
#define __ARCH_RISCV_INCLUDE_CH32H41X_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/* WCH CH32H41x family (CH32H415 / CH32H416 / CH32H417): dual QingKe
 * RV32IMAFCB-X cores (V3F and V5F) with single-precision FPU.  NuttX runs
 * on the V3F (boot master) core in machine mode.
 */

/* Core selection */

#if defined(CONFIG_CH32H41X_CORE_V3F)
#  define CH32H41X_CORE_V3F 1
#elif defined(CONFIG_CH32H41X_CORE_V5F)
#  define CH32H41X_CORE_V5F 1
#else
#  error "CH32H41x core (V3F or V5F) not selected"
#endif

/* Memory map.  The flash and RAM windows used by NuttX are configurable
 * because the family members differ in resources (CH32H417: 960KiB code
 * flash, 512KiB shared SRAM).  When both cores are used, the windows must
 * not overlap; the V3F window starts at 0x0 and the V5F window follows it.
 */

#define CH32H41X_FLASH_PBASE    0x00000000  /* Code flash (zero-wait, storage only) */
#define CH32H41X_FLASH_ALIAS    0x08000000  /* Flash programming alias */
#define CH32H41X_ITCM_PBASE    0x200a0000  /* ITCM, 128KiB (V5F) */
#define CH32H41X_DTCM_PBASE    0x200c0000  /* DTCM, 256KiB (V5F) */
#define CH32H41X_SRAM_PBASE    0x20100000  /* Shared SRAM */

#ifndef CONFIG_CH32H41X_FLASH_SIZE
#  define CONFIG_CH32H41X_FLASH_SIZE 0x80000   /* 512KiB V3F window default */
#endif

#ifndef CONFIG_CH32H41X_RAM_SIZE
#  define CONFIG_CH32H41X_RAM_SIZE   0x80000   /* 512KiB shared SRAM default */
#endif

#define CH32H41X_FLASH_SIZE     CONFIG_CH32H41X_FLASH_SIZE
#define CH32H41X_RAM_SIZE       CONFIG_CH32H41X_RAM_SIZE

#if defined(CH32H41X_CORE_V3F)
#  define CH32H41X_RAM_PBASE    CH32H41X_SRAM_PBASE
#elif defined(CH32H41X_CORE_V5F)
#  define CH32H41X_RAM_PBASE    CH32H41X_ITCM_PBASE
#endif

#endif /* __ARCH_RISCV_INCLUDE_CH32H41X_CHIP_H */
