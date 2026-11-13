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

/* Oneshot timer lower-half for TIM6/TIM7 (basic timers).
 *
 * chan 0 = TIM6, chan 1 = TIM7.  The counter runs at 10kHz (resolution
 * 100 microseconds per tick), so the maximum one-shot delay is
 * 65535 / 10kHz = 6.55 seconds.
 *
 * Register/bit names follow hardware/ch32h41x_tim.h (verified against
 * the RM): TIM7 is clocked from HB1PCENR bit5, its update interrupt is
 * PFIC line 101 (NuttX CH32H41X_IRQ_TIM7), UIF clears by writing 0 to
 * INTFR bit0.
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

/* TIM7 update interrupt (NuttX id; PFIC line 101) */

#define ONESHOT_TIM7_IRQ  (CH32H41X_IRQ_FIRST + 101)
#define ONESHOT_TIM6_IRQ  (CH32H41X_IRQ_FIRST + 100)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_oneshot_s
{
  struct oneshot_lowerhalf_s lower;   /* NuttX handle (first member) */
  uintptr_t base;                     /* timer register base */
  uint8_t timer;                      /* 0=TIM6, 1=TIM7 */
  uint8_t irq;                        /* NuttX IRQ id */
  bool running;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int ch32h41x_oneshot_maxdelay(FAR struct oneshot_lowerhalf_s *lower,
                                     FAR struct timespec *ts);
static int ch32h41x_oneshot_start(FAR struct oneshot_lowerhalf_s *lower,
                                  FAR const struct timespec *ts);
static int ch32h41x_oneshot_cancel(FAR struct oneshot_lowerhalf_s *lower,
                                   FAR struct timespec *ts);
static int ch32h41x_oneshot_current(FAR struct oneshot_lowerhalf_s *lower,
                                    FAR struct timespec *ts);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct oneshot_operations_s g_oneshot_ops =
{
  .max_delay = ch32h41x_oneshot_maxdelay,
  .start     = ch32h41x_oneshot_start,
  .cancel    = ch32h41x_oneshot_cancel,
  .current   = ch32h41x_oneshot_current,
};

static struct ch32h41x_oneshot_s g_oneshot[ONESHOT_NTIMERS] =
{
  { { &g_oneshot_ops }, ONESHOT_TIM6_BASE, 0, ONESHOT_TIM6_IRQ, false },
  { { &g_oneshot_ops }, ONESHOT_TIM7_BASE, 1, ONESHOT_TIM7_IRQ, false }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_oneshot_timer_arm
 *
 * Description:
 *   Arm the basic timer for 'ticks' 100us ticks and enable its update
 *   interrupt.
 *
 ****************************************************************************/

static void ch32h41x_oneshot_timer_arm(FAR struct ch32h41x_oneshot_s *priv,
                                       uint32_t ticks)
{
  uintptr_t base = priv->base;

  /* Disable while reconfiguring */

  modifyreg32(CH32H41X_TIM_DMAINTENR(base), CH32H41X_TIM_DIER_UIE, 0);
  modifyreg32(CH32H41X_TIM_CTLR1(base), CH32H41X_TIM_CR1_CEN, 0);

  /* 100us tick clock from HCLK; the update event latches PSC/ARR */

  putreg32(ONESHOT_PSC, CH32H41X_TIM_PSC(base));
  putreg32(ticks - 1, CH32H41X_TIM_ATRLR(base));
  modifyreg32(CH32H41X_TIM_CTLR1(base), 0, CH32H41X_TIM_CR1_CEN);
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

  modifyreg32(CH32H41X_TIM_INTFR(priv->base), CH32H41X_TIM_INT_UIF, 0);
  priv->running = false;

  oneshot_process_callback(&priv->lower);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_maxdelay
 ****************************************************************************/

static int ch32h41x_oneshot_maxdelay(FAR struct oneshot_lowerhalf_s *lower,
                                     FAR struct timespec *ts)
{
  uint64_t usec = (uint64_t)ONESHOT_MAXTICKS * 100ul;

  ts->tv_sec  = (time_t)(usec / 1000000ul);
  ts->tv_nsec = (long)((usec % 1000000ul) * 1000ul);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_start
 ****************************************************************************/

static int ch32h41x_oneshot_start(FAR struct oneshot_lowerhalf_s *lower,
                                  FAR const struct timespec *ts)
{
  FAR struct ch32h41x_oneshot_s *priv =
    (FAR struct ch32h41x_oneshot_s *)lower;
  uint64_t usec;
  uint64_t ticks;

  if (ts->tv_sec < 0 || ts->tv_nsec < 0)
    {
      return -EINVAL;
    }

  usec = ((uint64_t)ts->tv_sec * 1000000ul) +
         ((uint64_t)ts->tv_nsec / 1000ul);

  ticks = (usec + 99ul) / 100ul;   /* ceil to 100us ticks */
  if (ticks == 0)
    {
      ticks = 1;
    }

  if (ticks > ONESHOT_MAXTICKS)
    {
      return -EINVAL;
    }

  ch32h41x_oneshot_timer_arm(priv, (uint32_t)ticks);
  priv->running = true;
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_cancel
 ****************************************************************************/

static int ch32h41x_oneshot_cancel(FAR struct oneshot_lowerhalf_s *lower,
                                   FAR struct timespec *ts)
{
  FAR struct ch32h41x_oneshot_s *priv =
    (FAR struct ch32h41x_oneshot_s *)lower;
  uint32_t cnt;

  modifyreg32(CH32H41X_TIM_DMAINTENR(priv->base), CH32H41X_TIM_DIER_UIE, 0);
  modifyreg32(CH32H41X_TIM_CTLR1(priv->base), CH32H41X_TIM_CR1_CEN, 0);

  cnt = getreg32(CH32H41X_TIM_CNT(priv->base)) & ONESHOT_MAXTICKS;
  priv->running = false;

  if (ts != NULL)
    {
      uint64_t usec = (uint64_t)cnt * 100ul;

      ts->tv_sec  = (time_t)(usec / 1000000ul);
      ts->tv_nsec = (long)((usec % 1000000ul) * 1000ul);
    }

  return OK;
}

/****************************************************************************
 * Name: ch32h41x_oneshot_current
 ****************************************************************************/

static int ch32h41x_oneshot_current(FAR struct oneshot_lowerhalf_s *lower,
                                    FAR struct timespec *ts)
{
  FAR struct ch32h41x_oneshot_s *priv =
    (FAR struct ch32h41x_oneshot_s *)lower;
  uint32_t cnt = getreg32(CH32H41X_TIM_CNT(priv->base)) & ONESHOT_MAXTICKS;
  uint64_t usec = (uint64_t)cnt * 100ul;

  ts->tv_sec  = (time_t)(usec / 1000000ul);
  ts->tv_nsec = (long)((usec % 1000000ul) * 1000ul);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: oneshot_initialize
 *
 * Description:
 *   Initialize the TIM6/TIM7 basic timer as a oneshot lower-half driver.
 *   chan 0 selects TIM6, chan 1 selects TIM7.  The counter resolution is
 *   fixed at 100 microseconds (10kHz count clock).
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

  ret = irq_attach(priv->irq, ch32h41x_oneshot_isr, priv);
  if (ret != OK)
    {
      return NULL;
    }

  up_enable_irq(priv->irq);
  return &priv->lower;
}
#endif /* CONFIG_CH32H41X_ONESHOT */
