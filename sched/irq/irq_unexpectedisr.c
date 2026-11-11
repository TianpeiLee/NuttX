/****************************************************************************
 * sched/irq/irq_unexpectedisr.c
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
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>

#include "irq/irq.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Ring record of the last unexpected IRQ numbers, for post-mortem reads
 * over a debugger.  syslog() must NOT be called from here: on this
 * platform an unexpected interrupt can arrive while the console output
 * path holds the syslog lock, and logging from the interrupt context
 * then spins forever inside the syslog retry loop and hangs the whole
 * boot.  Disabling the storming line keeps the system alive. */

static volatile int g_unexp_count;
static volatile int g_unexp_irq[16];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: irq_unexpected_isr
 *
 * Description:
 *   An interrupt has been received for an IRQ that was never registered
 *   with the system.  Record the IRQ number, disable that line so it
 *   cannot storm, and keep running.  The recorded numbers survive in
 *   RAM (g_unexp_irq / g_unexp_count) for debugger read-back.
 *
 ****************************************************************************/

int irq_unexpected_isr(int irq, FAR void *context, FAR void *arg)
{
  UNUSED(context);
  UNUSED(arg);

  if (g_unexp_count < 16)
    {
      g_unexp_irq[g_unexp_count] = irq;
      g_unexp_count++;
    }

  /* Disable this line so an unexpected interrupt cannot storm.  A
   * driver that really needs it must attach before enabling. */

  up_disable_irq(irq);

  return OK;
}
