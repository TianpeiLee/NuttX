/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_oneshot.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* Oneshot timer lower-half for TIM6/TIM7 (basic timers), count-based
 * interface (CONFIG_ONESHOT_COUNT, selected by ARCH_CHIP_CH32H41X).
 *
 * chan 0 = TIM6, chan 1 = TIM7.  The counter runs at 10kHz (resolution
 * 100 microseconds per tick), so the maximum one-shot delay is
 * 65535 / 10kHz = 6.55 seconds.  Counts are the raw 10kHz counter ticks
 * (clkcnt_t is 64-bit; hardware counts are 16-bit).
 *
 * Register/bit names follow hardware/ch32h41x_tim.h (verified against
 * the RM): TIM7 is clocked from HB1PCENR bit5, its update interrupt is
 * PFIC line 101 (NuttX CH32H41X_IRQ_TIM7), UIF clears by writing 0 to
 * INTFR bit0.  The prescaler is buffered: PSC loads from its shadow only
 * on an update event, so a UG pulse (SWEVGR) must follow every PSC write.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/timers/oneshot.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_tim.h"

#include "ch32h41x_internal.h"

#ifdef CONFIG_CH32H41X_ONESHOT

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* chan 0 = TIM6, chan 1 = TIM7 */

#define ONESHOT_TIM6_BASE CH32H41X_TIM6_BASE
#define ONESHOT_TIM7_BASE CH32H41X_TIM7_BASE

#define ONESHOT_NTIMERS   2

/* Counter clock: 10kHz (100us per tick) from the 100MHz HCLK */

#define ONESHOT_TICK_HZ   10000ul
#define ONESHOT_PSC       ((ch32h41x_hclk_frequency() / ONESHOT_TICK_HZ) - 1)

#define ONESHOT_MAXTICKS  0xfffful

/* TIM layer index used by ch32h41x_tim_clock_enable()/uirq[]:
 * 5 = TIM6, 6 = TIM7.  The update interrupts are PFIC lines 100/101. */

#define ONESHOT_TIM7_IRQ  (CH32H41X_IRQ_FIRST + 101)
#define ONESHOT_TIM6_IRQ  (CH32H41X_IRQ_FIRST + 100)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_oneshot_s
{
  struct oneshot_lowerhalf_s lower;   /* NuttX handle (first member) */
  uintptr_t base;                     /* timer register base */
  uint8_t timer;                      /* TIM layer index: 5=TIM6, 6=TIM7 */
  uint8_t irq;                        /* NuttX IRQ id */
  bool running;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static clkcnt_t ch32h41x_oneshot_current(FAR struct oneshot_lowerhalf_s *l);
static void ch32h41x_oneshot_start(FAR struct oneshot_lowerhalf_s *l,
                                   clkcnt_t delta);
static void ch32h41x_oneshot_start_absolute(FAR struct oneshot_lowerhalf_s *l,
                                            clkcnt_t cnt);
static void ch32h41x_oneshot_cancel(FAR struct oneshot_lowerhalf_s *l);
static clkcnt_t ch32h41x_oneshot_max_delay(FAR struct oneshot_lowerhalf_s *l);
static int ch32h41x_oneshot_isr(int irq, FAR void *context, FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct oneshot_operations_s g_oneshot_ops =
{
  .current        = ch32h41x_oneshot_current,
  .start          = ch32h41x_oneshot_start,
  .start_absolute = ch32h41x_oneshot_start_absolute,
  .cancel         = ch32h41x_oneshot_cancel,
  .max_delay      = ch32h41x_oneshot_max_delay
};

static struct ch32h41x_oneshot_s g_oneshot[ONESHOT_NTIMERS] =
{
  { { &g_oneshot_ops }, ONESHOT_TIM6_BASE, 5, ONESHOT_TIM6_IRQ, false },
  { { &g_oneshot_ops }, ONESHOT_TIM7_BASE, 6, ONESHOT_TIM7_IRQ, false }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_oneshot_timer_arm
 *
 * Description:
 *   Arm the basic timer for 'ticks' 100us ticks and enable its update
 *   interrupt.  The prescaler is buffered, so a UG pulse loads the new
 *   PSC/ARR and the UIF it sets is cleared before re-enabling the source.
 *
 ****************************************************************************/

static void ch32h41x_oneshot_timer_arm(FAR struct ch32h41x_oneshot_s *priv,
                                       uint32_t ticks)
{
  uintptr_t base = priv->base;

  /* Disable while reconfiguring */

  modifyreg32(CH32H41X_TIM_DMAINTENR(base), CH32H41X_TIM_DIER_UIE, 0);
  modifyreg32(CH32H41X_TIM_CTLR1(base), CH32H41X_TIM_CR1_CEN, 0);

  /* 100us tick clock from HCLK; UG latches PSC/ARR into the shadows */

  putreg32(ONESHOT_PSC, CH32H41X_TIM_PSC(base));
  putreg32(ticks - 1, CH32H41X_TIM_ATRLR(base));
  putreg32(CH32H41X_TIM_EVG_UG, CH32H41X_TIM_SWEVGR(base));
  modifyreg32(CH32H41X_TIM_INTFR(base), CH32H41X_TIM_INT_UIF, 0);

  /* Arm and run */

  modifyreg32(CH32H41X_TIM_DMAINTENR(base), 0, CH32H41X_TIM_DIER_UIE);
  modifyreg32(CH32H41X_TIM_CTLR1(base), 0, CH32H41X_TIM_CR1_CEN);
}

/****************************************************************************
 * Name: ch32h41x_oneshot_isr
 *
 * Description:
 *   Update interrupt: clear the flag and deliver the callback.
 *
 ****************************************************************************/

static int ch32h41x_oneshot_isr(int irq, FAR void *context, FAR void *arg)
{
  FAR struct ch32h41x_oneshot_s *priv = (FAR struct ch32h41x_oneshot_s *)arg;

  modifyreg32(CH32H41X_TIM_DMAINTENR(priv->base),
              CH32H41X_TIM_DIER_UIE, 0);
  modifyreg32(CH32H41X_TIM_CTLR1(priv->base), CH32H41X_TIM_CR1_CEN, 0);
  modifyreg32(CH32H41X_TIM_INTFR(priv->base), CH32H41X_TIM_INT_UIF, 0);
  priv->running = false;

  oneshot_process_callback(&priv->lower);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_current
 *
 * Description:
 *   Return the raw 10kHz counter.  The hardware counter is 16-bit and
 *   only runs while a shot is armed, so the value is a short-window
 *   counter, not a monotonic system clock (same semantics as reading
 *   CNT directly; the upper layers only use it for cancel bookkeeping).
 *
 ****************************************************************************/

static clkcnt_t ch32h41x_oneshot_current(FAR struct oneshot_lowerhalf_s *l)
{
  FAR struct ch32h41x_oneshot_s *priv = (FAR struct ch32h41x_oneshot_s *)l;

  return getreg32(CH32H41X_TIM_CNT(priv->base)) & ONESHOT_MAXTICKS;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_start
 *
 * Description:
 *   Arm the timer to expire after 'delta' 10kHz ticks (clamped to the
 *   16-bit range).
 *
 ****************************************************************************/

static void ch32h41x_oneshot_start(FAR struct oneshot_lowerhalf_s *l,
                                   clkcnt_t delta)
{
  FAR struct ch32h41x_oneshot_s *priv = (FAR struct ch32h41x_oneshot_s *)l;
  uint32_t ticks;

  if (delta < 1)
    {
      ticks = 1;
    }
  else if (delta > (clkcnt_t)(ONESHOT_MAXTICKS - 1))
    {
      ticks = ONESHOT_MAXTICKS - 1;
    }
  else
    {
      ticks = (uint32_t)delta;
    }

  ch32h41x_oneshot_timer_arm(priv, ticks);
  priv->running = true;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_start_absolute
 ****************************************************************************/

static void ch32h41x_oneshot_start_absolute(FAR struct oneshot_lowerhalf_s *l,
                                            clkcnt_t cnt)
{
  clkcnt_t now = ch32h41x_oneshot_current(l);
  clkcnt_t delta = cnt - now;

  if (delta > cnt)
    {
      /* Wrapped (expected behind current): fire immediately */

      delta = 1;
    }

  ch32h41x_oneshot_start(l, delta);
}

/****************************************************************************
 * Name: ch32h41x_oneshot_cancel
 ****************************************************************************/

static void ch32h41x_oneshot_cancel(FAR struct oneshot_lowerhalf_s *l)
{
  FAR struct ch32h41x_oneshot_s *priv = (FAR struct ch32h41x_oneshot_s *)l;

  modifyreg32(CH32H41X_TIM_DMAINTENR(priv->base),
              CH32H41X_TIM_DIER_UIE, 0);
  modifyreg32(CH32H41X_TIM_CTLR1(priv->base), CH32H41X_TIM_CR1_CEN, 0);
  priv->running = false;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_max_delay
 ****************************************************************************/

static clkcnt_t ch32h41x_oneshot_max_delay(FAR struct oneshot_lowerhalf_s *l)
{
  return (clkcnt_t)(ONESHOT_MAXTICKS - 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: oneshot_initialize
 *
 * Description:
 *   Initialize the TIM6/TIM7 basic timer as a count-based oneshot
 *   lower-half driver.  chan 0 selects TIM6, chan 1 selects TIM7.  The
 *   counter resolution is fixed at 100 microseconds (10kHz count clock).
 *
 * Returned Value:
 *   Non-NULL lower-half instance on success; NULL on failure.
 *
 ****************************************************************************/

FAR struct oneshot_lowerhalf_s *oneshot_initialize(int chan,
                                                   uint16_t resolution)
{
  FAR struct ch32h41x_oneshot_s *priv;
  int ret;

  if (chan < 0 || chan >= ONESHOT_NTIMERS)
    {
      return NULL;
    }

  priv = &g_oneshot[chan];

  /* Bus clock + update interrupt on the shared TIM infrastructure */

  ch32h41x_tim_clock_enable(priv->timer, true);
  modifyreg32(CH32H41X_TIM_DMAINTENR(priv->base), CH32H41X_TIM_DIER_UIE, 0);
  modifyreg32(CH32H41X_TIM_CTLR1(priv->base), CH32H41X_TIM_CR1_CEN, 0);
  putreg32(ONESHOT_PSC, CH32H41X_TIM_PSC(priv->base));
  putreg32(ONESHOT_MAXTICKS - 1, CH32H41X_TIM_ATRLR(priv->base));

  /* UG loads the prescaler shadow; clear the update flag it sets */

  putreg32(CH32H41X_TIM_EVG_UG, CH32H41X_TIM_SWEVGR(priv->base));
  modifyreg32(CH32H41X_TIM_INTFR(priv->base), CH32H41X_TIM_INT_UIF, 0);

  /* Count-based interface: publish the 10kHz count clock */

  oneshot_count_init(&priv->lower, ONESHOT_TICK_HZ);

  ret = irq_attach(priv->irq, ch32h41x_oneshot_isr, priv);
  if (ret != OK)
    {
      return NULL;
    }

  up_enable_irq(priv->irq);
  return &priv->lower;
}
#endif /* CONFIG_CH32H41X_ONESHOT */
