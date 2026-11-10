/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_exti.c
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
#include <assert.h>
#include <errno.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_gpio.h"
#include "hardware/ch32h41x_exti.h"

#include "ch32h41x_internal.h"

#ifdef CONFIG_CH32H41X_EXTI

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Callback signature delivered from EXTI context */

typedef int (*ch32h41x_exti_callback_t)(void *arg);

struct ch32h41x_exti_s
{
  ch32h41x_exti_callback_t callback;  /* ISR for this line (NULL = none) */
  void *arg;                          /* ISR argument */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* One slot per EXTI line 0..15.  Lines 0-7 share the EXTI7_0 interrupt,
 * lines 8-15 the EXTI15_8 interrupt. */

static struct ch32h41x_exti_s g_exti[16];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_exti_isr
 ****************************************************************************/

static int ch32h41x_exti_isr(int irq, void *context, void *arg)
{
  uint32_t pending;
  int line;
  int base;
  ch32h41x_exti_callback_t callback;
  void *cbarg;
  int ret = OK;

  /* Shared handler for both the [7:0] and [15:8] vector.  Dispatch each
   * pending line, clearing the flag by writing 1. */

  base = (irq == CH32H41X_IRQ_EXTI15_8) ? 8 : 0;
  pending = getreg32(CH32H41X_EXTI_INTFR);

  for (line = 0; line < 8; line++)
    {
      uint32_t mask = CH32H41X_EXTI_LINE(base + line);

      if ((pending & mask) != 0)
        {
          putreg32(mask, CH32H41X_EXTI_INTFR);

          callback = g_exti[base + line].callback;
          cbarg    = g_exti[base + line].arg;
          if (callback != NULL)
            {
              ret |= callback(cbarg);
            }
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_exti_initialize
 *
 * Description:
 *   Attach the two shared EXTI interrupt vectors.  Called once from
 *   ch32h41x_gpioinit() when EXTI support is enabled.
 *
 ****************************************************************************/

void ch32h41x_exti_initialize(void)
{
  irq_attach(CH32H41X_IRQ_EXTI7_0, ch32h41x_exti_isr, NULL);
  up_enable_irq(CH32H41X_IRQ_EXTI7_0);

  irq_attach(CH32H41X_IRQ_EXTI15_8, ch32h41x_exti_isr, NULL);
  up_enable_irq(CH32H41X_IRQ_EXTI15_8);
}

/****************************************************************************
 * Name: ch32h41x_exti_attach
 *
 * Description:
 *   Bind a callback to the EXTI line of the given pin and route the line
 *   to the pin's GPIO port.  The pin must already be configured as an
 *   input by the caller.
 *
 *   trigger: 0 = rising edge, 1 = falling edge, 2 = both edges
 *
 ****************************************************************************/

int ch32h41x_exti_attach(uint8_t port, uint8_t pin, uint8_t trigger,
                         ch32h41x_exti_callback_t callback, void *cbarg)
{
  irqstate_t flags;
  uintptr_t reg;
  uint32_t shift;

  if (port >= CH32H41X_GPIO_NPORTS || pin > 15 ||
      (trigger > 2))
    {
      return -EINVAL;
    }

  flags = enter_critical_section();

  /* Route the line to the port */

  reg   = CH32H41X_AFIO_EXTICR_REG(pin);
  shift = CH32H41X_AFIO_EXTICR_SHIFT(pin);
  modifyreg32(reg, CH32H41X_AFIO_EXTICR_MASK(pin),
              (uint32_t)port << shift);

  /* Configure the trigger edges */

  if (trigger == 0 || trigger == 2)
    {
      modifyreg32(CH32H41X_EXTI_RTENR, 0, CH32H41X_EXTI_LINE(pin));
    }
  else
    {
      modifyreg32(CH32H41X_EXTI_RTENR, CH32H41X_EXTI_LINE(pin), 0);
    }

  if (trigger == 1 || trigger == 2)
    {
      modifyreg32(CH32H41X_EXTI_FTENR, 0, CH32H41X_EXTI_LINE(pin));
    }
  else
    {
      modifyreg32(CH32H41X_EXTI_FTENR, CH32H41X_EXTI_LINE(pin), 0);
    }

  /* Clear any stale pending flag, then bind the callback */

  putreg32(CH32H41X_EXTI_LINE(pin), CH32H41X_EXTI_INTFR);
  g_exti[pin].callback = callback;
  g_exti[pin].arg      = cbarg;

  leave_critical_section(flags);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_exti_enable
 *
 * Description:
 *   Mask / unmask the EXTI interrupt line (INTENR).  Also clears the
 *   pending flag on unmask so a disabled period cannot queue events.
 *
 ****************************************************************************/

void ch32h41x_exti_enable(uint8_t pin, bool enable)
{
  irqstate_t flags;

  DEBUGASSERT(pin < 16);

  flags = enter_critical_section();

  if (enable)
    {
      putreg32(CH32H41X_EXTI_LINE(pin), CH32H41X_EXTI_INTFR);
      modifyreg32(CH32H41X_EXTI_INTENR, 0, CH32H41X_EXTI_LINE(pin));
    }
  else
    {
      modifyreg32(CH32H41X_EXTI_INTENR, CH32H41X_EXTI_LINE(pin), 0);
    }

  leave_critical_section(flags);
}

#endif /* CONFIG_CH32H41X_EXTI */
