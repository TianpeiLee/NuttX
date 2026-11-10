/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_start.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/init.h>
#include <nuttx/arch.h>
#include <arch/board/board.h>

#include "riscv_internal.h"
#include "ch32h41x_internal.h"
#include "chip.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern void ch32h41x_lowsetup(void);
void ch32h41x_boardinitialize(void);

/****************************************************************************
 * Name: ch32h41x_start
 *
 * Description:
 *   The C-level boot sequence.  Entered from ch32h41x_head.S after the
 *   flash-resident sections have been copied to SRAM and the core CSRs
 *   configured.  Never returns.
 *
 ****************************************************************************/

void ch32h41x_start(void)
{
  /* Enable the single-precision FPU (mstatus.FS = INITIAL).  Must come
   * first: with CONFIG_ARCH_FPU the common context switch code assumes
   * the FPU state is valid.
   */

  riscv_fpuconfig();

  /* Bring up the console as early as possible, while the chip still runs
   * from the reset clock (25MHz HSI): enable the AFIO/GPIO clocks, then
   * the console UART pins and peripheral.  The baud divider programmed
   * here matches the reset clock, so output works even if the PLL
   * bring-up below fails.
   */

  ch32h41x_gpioinit();
  ch32h41x_lowsetup();

  /* Configure the chip clocks (HSE/PLL bring-up on the V3F boot master
   * core, frequency identification on the V5F).  Non-fatal: on HSE/PLL
   * failure the chip keeps running from the HSI and the console stays
   * usable.
   */

  ch32h41x_clockconfig();

  /* Early serial initialization (polled console).  Reconfigures the
   * console baud rate for the final HCLK frequency.
   */

#ifdef USE_EARLYSERIALINIT
  riscv_earlyserialinit();
#endif

  /* Perform board-specific initialization */

  ch32h41x_boardinitialize();

  /* Start the NuttX scheduler */

  nx_start();

  /* nx_start() should never return */

  for (; ; )
    {
    }
}
