/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_capture.c
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
 * PWM-input capture on TIM2 channel 1 at PA0 (CC1 latches rising edges;
 * the wrapped counter difference gives the period).  The loopback jumper
 * is PA8 (TIM1_CH1 PWM) -> PA0 (the EVT Input_Capture demo wiring).
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
#include <nuttx/timers/capture.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_tim.h"

#include "ch32h41x_internal.h"

#ifdef CONFIG_CH32H41X_TIM2_CAPTURE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CAP_TIM_IDX      1             /* TIM2 */
#define CAP_TIMCLK       1000000ul     /* counter clock after PSC */

/* INTFR layout note (hardware-verified on CH32H417 TIM2 by register
 * probing): the capture flags sit at the F1-standard positions
 * (CC1IF=1, CC2IF=2, CC3IF=3, CC4IF=4).  Earlier "shifted flags"
 * readings were actually the flags of the unconfigured output channels
 * (CCR=0 matches at every counter wrap), not a shifted layout.  Only
 * enable/monitor the CC1 position; the other channels' flags are left
 * alone so their wrap-rate false edges cannot pollute the count. */

#define CAP_INT_CC1IE  (1ul << 1)
#define CAP_FLG_CC1IF  (1ul << 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_capture_s
{
  struct cap_lowerhalf_s lower;  /* must be first */
  bool running;
  uint32_t freq;                 /* measured frequency, Hz */
  uint8_t duty;                  /* measured duty, percent */
  uint32_t edges;                /* captured rising edges */
  uint16_t last_rise;            /* last CC1 value */
  uint32_t psc;                  /* active prescaler (for duty math) */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int ch32h41x_cap_start(FAR struct cap_lowerhalf_s *lower);
static int ch32h41x_cap_stop(FAR struct cap_lowerhalf_s *lower);
static int ch32h41x_cap_getduty(FAR struct cap_lowerhalf_s *lower,
                                FAR uint8_t *duty);
static int ch32h41x_cap_getfreq(FAR struct cap_lowerhalf_s *lower,
                                FAR uint32_t *freq);
static int ch32h41x_cap_getedges(FAR struct cap_lowerhalf_s *lower,
                                 FAR uint32_t *edges);
static int ch32h41x_cap_isr(int irq, void *context, void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct cap_ops_s g_cap_ops =
{
  .start    = ch32h41x_cap_start,
  .stop     = ch32h41x_cap_stop,
  .getduty  = ch32h41x_cap_getduty,
  .getfreq  = ch32h41x_cap_getfreq,
  .getedges = ch32h41x_cap_getedges,
  .ioctl    = NULL,
};

static struct ch32h41x_capture_s g_capture;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_cap_isr
 *
 * Description:
 *   Capture event: CC1 = rising edge of TI1 (PA0).  The flag is cleared
 *   by writing 0 to the INTFR bit.
 *
 ****************************************************************************/

static int ch32h41x_cap_isr(int irq, void *context, void *arg)
{
  FAR struct ch32h41x_capture_s *priv = &g_capture;
  uintptr_t base = CH32H41X_TIM2_BASE;
  uint32_t intfr = getreg32(CH32H41X_TIM_INTFR(base));
  uint16_t c2;
  uint16_t period;

  if ((intfr & CAP_FLG_CC1IF) != 0)
    {
      /* CC1 captures the counter at each rising edge of TI1 (PA0) */

      c2 = getreg16(CH32H41X_TIM_CH1CVR(base));

      if (priv->edges > 0)
        {
          period = (uint16_t)(c2 - priv->last_rise);
          if (period > 0)
            {
              priv->freq = CAP_TIMCLK / period;
            }
        }

      priv->last_rise = c2;
      priv->edges++;

      modifyreg32(CH32H41X_TIM_INTFR(base),
                  CAP_FLG_CC1IF | CH32H41X_TIM_INT_CC1OF, 0);
    }

  return OK;
}

/****************************************************************************
 * Name: ch32h41x_cap_start
 *
 * Description:
 *   Configure TIM2 CH1 in input-capture mode on TI1 (PA0), counting
 *   rising edges via the wrapped counter difference in the ISR.
 *
 ****************************************************************************/

static int ch32h41x_cap_start(FAR struct cap_lowerhalf_s *lower)
{
  uintptr_t base = CH32H41X_TIM2_BASE;
  uint32_t psc;
  uint32_t div;

  ch32h41x_tim_clock_enable(CAP_TIM_IDX, true);

  /* Prescaler: counter clock = HCLK/(psc+1).  Choose psc for ~1MHz. */

  div = ch32h41x_hclk_frequency() / 1000000ul;
  psc = (div > 0) ? div - 1 : 0;
  g_capture.psc = psc;
  putreg16((uint16_t)psc, CH32H41X_TIM_PSC(base));
  putreg16(0xffff, CH32H41X_TIM_ATRLR(base));

  /* Generate an update event so the written PSC loads into the shadow
   * (without it the prescaler stays at reset and the counter runs at the
   * full timer clock). */

  putreg32(CH32H41X_TIM_EVG_UG, CH32H41X_TIM_SWEVGR(base));

  /* CC1S = 01: CC1 input on TI1 (PA0), no filter; capture on rising */

  modifyreg32(CH32H41X_TIM_CHCTLR1(base),
              CH32H41X_TIM_CCMR_CC1S_MASK |
              CH32H41X_TIM_CCMR_IC1F_MASK,
              CH32H41X_TIM_CCMR_CC1S_IN_TI1);

  modifyreg32(CH32H41X_TIM_CCER(base),
              CH32H41X_TIM_CCE_CC1P | CH32H41X_TIM_CCE_CC1E,
              CH32H41X_TIM_CCE_CC1E);

  /* Free-running counter (no slave mode): the ISR computes the period
   * from the wrapped difference of consecutive rising edges.  SMCFGR is
   * written explicitly because stale reset-mode configuration survives
   * core-only resets (wlink_reset_resume) and would zero every capture. */

  putreg16(0, CH32H41X_TIM_SMCFGR(base));

  /* The counter free-runs over the full 16-bit window; the ISR computes
   * the period from the wrapped difference of consecutive rising edges. */

  /* Enable the capture interrupts */

  modifyreg32(CH32H41X_TIM_DMAINTENR(base), 0,
              CAP_INT_CC1IE);
  /* TIM1 routes CC interrupts through its dedicated TIM1CC vector;
   * the other timers use their single update vector. */

  ch32h41x_tim_irqattach(CAP_TIM_IDX, 0, ch32h41x_cap_isr);

  g_capture.edges = 0;
  g_capture.freq = 0;
  g_capture.duty = 0;

  /* Start the counter */

  modifyreg32(CH32H41X_TIM_CTLR1(base), 0,
              CH32H41X_TIM_CR1_CEN | CH32H41X_TIM_CR1_URS);

  g_capture.running = true;
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_cap_stop
 ****************************************************************************/

static int ch32h41x_cap_stop(FAR struct cap_lowerhalf_s *lower)
{
  uintptr_t base = CH32H41X_TIM2_BASE;

  modifyreg32(CH32H41X_TIM_CTLR1(base), CH32H41X_TIM_CR1_CEN, 0);
  modifyreg32(CH32H41X_TIM_DMAINTENR(base),
              CAP_INT_CC1IE, 0);
  ch32h41x_tim_irqattach(CAP_TIM_IDX, 0, NULL);

  g_capture.running = false;
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_cap_getduty / getfreq / getedges
 ****************************************************************************/

static int ch32h41x_cap_getduty(FAR struct cap_lowerhalf_s *lower,
                                FAR uint8_t *duty)
{
  *duty = g_capture.duty;
  return OK;
}

static int ch32h41x_cap_getfreq(FAR struct cap_lowerhalf_s *lower,
                                FAR uint32_t *freq)
{
  *freq = g_capture.freq;
  return OK;
}

static int ch32h41x_cap_getedges(FAR struct cap_lowerhalf_s *lower,
                                 FAR uint32_t *edges)
{
  *edges = g_capture.edges;
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_capture_initialize
 *
 * Description:
 *   Register the TIM2 PWM-input capture as /dev/capture0.
 *
 ****************************************************************************/

int ch32h41x_capture_initialize(void)
{
  g_capture.lower.ops = &g_cap_ops;
  return cap_register("/dev/capture0", &g_capture.lower);
}

#endif /* CONFIG_CH32H41X_TIM2_CAPTURE */
