/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_tim.c
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
#include <stdbool.h>
#include <errno.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"

#include "ch32h41x_internal.h"

#ifdef CONFIG_CH32H41X_TIM

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Update-interrupt callbacks, one slot per timer
 * (0=TIM1 .. 4=TIM5, 5=TIM6, 6=TIM7, 7=TIM8, 8=TIM9, 9=TIM10,
 * 10=TIM11, 11=TIM12) */

static timisr_t g_timisr[12];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_timisr
 *
 * Description:
 *   Shared interrupt handler for the timer vectors.  Dispatches to the
 *   driver registered for the timer this vector belongs to.
 *
 ****************************************************************************/

static int ch32h41x_timisr(int irq, void *context, void *arg)
{
  int timer = (int)(intptr_t)arg;

  if (timer >= 0 && timer < 12 && g_timisr[timer] != NULL)
    {
      return g_timisr[timer](irq, context, arg);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_tim_clock_enable
 *
 * Description:
 *   Enable/disable the bus clock for the given timer (0 = TIM1 .. 4 =
 *   TIM5).  TIM1 lives on HB2, TIM2..TIM5 on HB1.
 *
 ****************************************************************************/

void ch32h41x_tim_clock_enable(int timer, bool enable)
{
  uintptr_t reg;
  uint32_t mask;

  switch (timer)
    {
      case 0:
        reg = CH32H41X_RCC_HB2PCENR;
        mask = CH32H41X_RCC_HB2_TIM1;
        break;

      case 1:
        reg = CH32H41X_RCC_HB1PCENR;
        mask = CH32H41X_RCC_HB1_TIM2;
        break;

      case 2:
        reg = CH32H41X_RCC_HB1PCENR;
        mask = CH32H41X_RCC_HB1_TIM3;
        break;

      case 3:
        reg = CH32H41X_RCC_HB1PCENR;
        mask = CH32H41X_RCC_HB1_TIM4;
        break;

      case 4:
        reg = CH32H41X_RCC_HB1PCENR;
        mask = CH32H41X_RCC_HB1_TIM5;
        break;

      case 5:
        reg = CH32H41X_RCC_HB1PCENR;
        mask = CH32H41X_RCC_HB1_TIM6;
        break;

      case 6:
        reg = CH32H41X_RCC_HB1PCENR;
        mask = CH32H41X_RCC_HB1_TIM7;
        break;

      case 7:
        reg = CH32H41X_RCC_HB2PCENR;
        mask = CH32H41X_RCC_HB2_TIM8;
        break;

      case 8:
        reg = CH32H41X_RCC_HB2PCENR;
        mask = CH32H41X_RCC_HB2_TIM9;
        break;

      case 9:
        reg = CH32H41X_RCC_HB2PCENR;
        mask = CH32H41X_RCC_HB2_TIM10;
        break;

      case 10:
        reg = CH32H41X_RCC_HB2PCENR;
        mask = CH32H41X_RCC_HB2_TIM11;
        break;

      case 11:
        reg = CH32H41X_RCC_HB2PCENR;
        mask = CH32H41X_RCC_HB2_TIM12;
        break;

      default:
        return;
    }

  if (enable)
    {
      modifyreg32(reg, 0, mask);
    }
  else
    {
      modifyreg32(reg, mask, 0);
    }
}

/****************************************************************************
 * Name: ch32h41x_tim_irqattach
 *
 * Description:
 *   Attach (or detach with NULL) the per-timer interrupt callback and
 *   enable the vector.  irqsel selects the timer interrupt vector:
 *   0 = update, 1 = capture/compare (TIM1 only).
 *
 ****************************************************************************/

int ch32h41x_tim_irqattach(int timer, int irqsel, timisr_t isr)
{
  static const int uirq[12] =
  {
    CH32H41X_IRQ_TIM1UP, CH32H41X_IRQ_TIM2, CH32H41X_IRQ_TIM3,
    CH32H41X_IRQ_TIM4, CH32H41X_IRQ_TIM5, CH32H41X_IRQ_TIM6,
    CH32H41X_IRQ_TIM7, CH32H41X_IRQ_TIM8UP, CH32H41X_IRQ_TIM9,
    CH32H41X_IRQ_TIM10, CH32H41X_IRQ_TIM11, CH32H41X_IRQ_TIM12,
  };

  static const int ccirq[12] =
  {
    CH32H41X_IRQ_TIM1CC, 0, 0, 0, 0, 0, 0, CH32H41X_IRQ_TIM8CC,
    0, 0, 0, 0,
  };

  int irq;

  if (timer < 0 || timer > 11)
    {
      return -EINVAL;
    }

  irq = (irqsel == 0) ? uirq[timer] : ccirq[timer];
  if (irq == 0)
    {
      return -ENOTSUP;
    }

  g_timisr[timer] = isr;

  if (isr != NULL)
    {
      irq_attach(irq, ch32h41x_timisr, (FAR void *)(intptr_t)timer);
      up_enable_irq(irq);
    }
  else
    {
      up_disable_irq(irq);
    }

  return OK;
}

#endif /* CONFIG_CH32H41X_TIM */
