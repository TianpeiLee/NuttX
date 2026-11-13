/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_timerisr.c
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
#include <time.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_sysctl.h"
#include "ch32h41x_internal.h"
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The V3F core uses SysTick0 as the NuttX system timer; the V5F uses
 * SysTick1 (the two cores have separate timers, the ISR status lives in
 * SysTick0->ISR for both).
 */

#ifdef CONFIG_CH32H41X_CORE_V3F
#  define CH32H41X_SYSTICK_BASE   CH32H41X_SYSTICK0_BASE
#  define CH32H41X_SYSTICK_IRQ    CH32H41X_IRQ_SYSTICK0
#else
#  define CH32H41X_SYSTICK_BASE   CH32H41X_SYSTICK1_BASE
#  define CH32H41X_SYSTICK_IRQ    CH32H41X_IRQ_SYSTICK1
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_timerisr
 *
 * Description:
 *   The system timer interrupt handler.  The compare flag is cleared by
 *   writing 0 to the ISR bit; the timer reloads the counter automatically
 *   when STRE is set, so no further reprogramming is required.
 *
 ****************************************************************************/

volatile unsigned long g_tick_count;
volatile unsigned int g_mie_debug;
volatile unsigned int g_stkctlr_debug;

static int ch32h41x_timerisr(int irq, void *context, void *arg)
{
  g_tick_count++;
  /* Clear the compare interrupt flag (write 0 to clear) */

  modifyreg32(CH32H41X_SYSTICK_BASE + CH32H41X_STK_ISR,
              CH32H41X_STK_ISR_UIF, 0);

  /* Process timer intervals */

  nxsched_process_timer();
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_timer_initialize
 *
 * Description:
 *   This function is called during start-up to initialize the system
 *   timer interrupt.  The core SysTick is clocked by HCLK and configured
 *   for automatic reload at the NuttX tick interval.
 *
 ****************************************************************************/

void up_timer_initialize(void)
{
  uint32_t interval =
    ch32h41x_hclk_frequency() / (1000000 / CONFIG_USEC_PER_TICK);

  /* Attach and enable the system timer interrupt */

  irq_attach(CH32H41X_SYSTICK_IRQ, ch32h41x_timerisr, NULL);
  up_enable_irq(CH32H41X_SYSTICK_IRQ);

  /* Tick-delivery diagnostics: capture the core interrupt state
   * right after arming the SysTick line */

  __asm__ __volatile__("csrr %0, mie" : "=r"(g_mie_debug));
  g_stkctlr_debug = getreg32(CH32H41X_SYSTICK_BASE +
                              CH32H41X_STK_CTLR);

  /* Reset the timer and program the first compare interval */

  putreg32(0, CH32H41X_SYSTICK_BASE + CH32H41X_STK_CTLR);
  putreg32(0, CH32H41X_SYSTICK_BASE + CH32H41X_STK_CNT);
  putreg32(interval - 1, CH32H41X_SYSTICK_BASE + CH32H41X_STK_CMP);
  modifyreg32(CH32H41X_SYSTICK_BASE + CH32H41X_STK_ISR,
              CH32H41X_STK_ISR_UIF, 0);

  /* Enable: HCLK source, auto reload, interrupt, timer */

  putreg32(CH32H41X_STK_CTLR_STE | CH32H41X_STK_CTLR_STIE |
           CH32H41X_STK_CTLR_STCLK | CH32H41X_STK_CTLR_STRE,
           CH32H41X_SYSTICK_BASE + CH32H41X_STK_CTLR);
}
