/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_irq_dispatch.c
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

#include <nuttx/irq.h>
#include <nuttx/arch.h>

#include "riscv_internal.h"
#include "chip.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: riscv_dispatch_irq
 *
 * Description:
 *   Convert the raw mcause value into a NuttX irq number and deliver the
 *   interrupt.  Synchronous exceptions pass through with their exception
 *   code (0..15); asynchronous interrupts are offset by RISCV_IRQ_ASYNC.
 *
 ****************************************************************************/

void *riscv_dispatch_irq(uintreg_t vector, uintreg_t *regs)
{
  int irq = vector & RISCV_IRQ_MASK;

  /* If this is an asynchronous interrupt, add the async offset */

  if ((vector & RISCV_IRQ_BIT) != 0)
    {
      irq += RISCV_IRQ_ASYNC;
    }

  /* Deliver the IRQ.  The PFIC clears the pending bit automatically when
   * an interrupt is taken, so there is no acknowledgement step here.
   */

  regs = riscv_doirq(irq, regs);

  return regs;
}
