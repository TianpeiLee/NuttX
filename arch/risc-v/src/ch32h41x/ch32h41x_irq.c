/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_irq.c
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
#include <stdio.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <arch/csr.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_sysctl.h"

#include "chip.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_irqinitialize
 ****************************************************************************/

void up_irqinitialize(void)
{
  /* Disable Machine interrupts */

  up_irq_save();

#if defined(CONFIG_STACK_COLORATION) && CONFIG_ARCH_INTERRUPTSTACK > 15
  /* Colorize the interrupt stack for debug purposes */

  size_t intstack_size = (CONFIG_ARCH_INTERRUPTSTACK & ~15);
  riscv_stack_color(g_intstackalloc, intstack_size);
#endif

  /* Route every peripheral interrupt (vector indices above 31) to this
   * core.  In the V3F configuration this is the boot master core; when
   * the V5F is brought up later it must re-route the IRQs it owns.
   */

#ifdef CONFIG_CH32H41X_CORE_V3F
  {
    int i;

    for (i = 32; i <= 148; i++)
      {
        putreg8(0, CH32H41X_PFIC_IALLOCR(i));
      }
  }
#endif

  /* Disable the interrupt threshold so all priorities are accepted */

  putreg32(0, CH32H41X_PFIC_ITHRESDR);

  /* Attach the common interrupt handler */

  riscv_exception_attach();

#ifndef CONFIG_SUPPRESS_INTERRUPTS

  /* And finally, enable interrupts */

  riscv_color_intstack();
  up_irq_enable();
#endif
}

/****************************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   Disable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_disable_irq(int irq)
{
  int index = irq - RISCV_IRQ_ASYNC;

  DEBUGASSERT(index >= 0 && index <= 148);

  putreg32(CH32H41X_PFIC_IRQBIT(index),
           CH32H41X_PFIC_IRER(CH32H41X_PFIC_IRQWORD(index)));
}

/****************************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   Enable the IRQ specified by 'irq'
 *
 ****************************************************************************/

void up_enable_irq(int irq)
{
  int index = irq - RISCV_IRQ_ASYNC;

  DEBUGASSERT(index >= 0 && index <= 148);

  putreg32(CH32H41X_PFIC_IRQBIT(index),
           CH32H41X_PFIC_IENR(CH32H41X_PFIC_IRQWORD(index)));
}

/****************************************************************************
 * Name: up_irq_enable
 *
 * Description:
 *   Return the current interrupt state and enable interrupts
 *
 ****************************************************************************/

irqstate_t up_irq_enable(void)
{
  /* Read mstatus & set machine interrupt enable (MIE) in mstatus */

  return READ_AND_SET_CSR(CSR_MSTATUS, MSTATUS_MIE);
}

/****************************************************************************
 * Name: up_systemreset
 *
 * Description:
 *   Reset the microcontroller.  Writing the PFIC system reset request
 *   key triggers a full chip reset.
 *
 ****************************************************************************/

void up_systemreset(void)
{
  putreg32(CH32H41X_PFIC_CFGR_KEY | CH32H41X_PFIC_CFGR_SYSRST,
           CH32H41X_PFIC_CFGR);

  /* Spin until the reset takes effect */

  for (; ; )
    {
      asm volatile ("wfi");
    }
}
