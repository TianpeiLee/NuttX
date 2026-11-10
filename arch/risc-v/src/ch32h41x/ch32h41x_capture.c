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
 * PWM-input capture on TIM2 channel 1 (PWM input mode: CC1 latches the
 * period on rising edges, CC2 latches the duty time on falling edges).
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

#define CAP_TIM_IDX 1                  /* TIM2 */

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
 *   Capture events: CC1 = rising edge (period), CC2 = falling edge (high
 *   time).  Both flags are cleared by writing 0 to the INTFR bits.
 *
 ****************************************************************************/

static int ch32h41x_cap_isr(int irq, void *context, void *arg)
{
  FAR struct ch32h41x_capture_s *priv = &g_capture;
  uintptr_t base = CH32H41X_TIM2_BASE;
  uint32_t intfr = getreg32(CH32H41X_TIM_INTFR(base));
  uint16_t c1;
  uint16_t c2;
  uint16_t period;
  uint16_t high;

  if ((intfr & CH32H41X_TIM_INT_CC1IF) != 0)
    {
      c1 = getreg16(CH32H41X_TIM_CH1CVR(base));

      if (priv->edges > 0)
        {
          period = (uint16_t)(c1 - priv->last_rise);
          if (period > 0)
            {
              priv->freq = ch32h41x_hclk_frequency() / period;
            }
        }

      priv->last_rise = c1;
      priv->edges++;

      modifyreg32(CH32H41X_TIM_INTFR(base), CH32H41X_TIM_INT_CC1IF, 0);
    }

  if ((intfr & CH32H41X_TIM_INT_CC2IF) != 0)
    {
      c2 = getreg16(CH32H41X_TIM_CH2CVR(base));
      high = (uint16_t)(c2 - priv->last_rise);

      if (priv->freq > 0)
        {
          priv->duty = (uint8_t)(((uint64_t)high * 100) /
                                 (ch32h41x_hclk_frequency() / priv->freq));
          if (priv->duty > 100)
            {
              priv->duty = 100;
            }
        }

      modifyreg32(CH32H41X_TIM_INTFR(base), CH32H41X_TIM_INT_CC2IF, 0);
    }

  return OK;
}

/****************************************************************************
 * Name: ch32h41x_cap_start
 *
 * Description:
 *   Configure TIM2 in PWM-input mode: CC1 captures TI1 rising edges,
 *   CC2 captures TI1 falling edges, slave-mode reset on rising edge so
 *   CNT wraps with the input period.
 *
 ****************************************************************************/

static int ch32h41x_cap_start(FAR struct cap_lowerhalf_s *lower)
{
  uintptr_t base = CH32H41X_TIM2_BASE;
  uint32_t psc;
  uint32_t div;

  ch32h41x_tim_clock_enable(CAP_TIM_IDX, true);

  /* Prescaler: aim for a counter clock of about 10kHz*65536 so that both
   * kHz-range PWM frequencies and their periods fit the 16-bit window:
   * counter clock = HCLK/(psc+1).  Choose psc for ~1MHz counter clock. */

  div = ch32h41x_hclk_frequency() / 1000000ul;
  psc = (div > 0) ? div - 1 : 0;
  putreg16((uint16_t)psc, CH32H41X_TIM_PSC(base));
  putreg16(0xffff, CH32H41X_TIM_ATRLR(base));

  /* CC1S = 01: CC1 input on TI1;  CC2S = 10: CC2 input on TI1 (via CF2) */

  modifyreg32(CH32H41X_TIM_CHCTLR1(base),
              CH32H41X_TIM_CCMR_CC1S_MASK | CH32H41X_TIM_CCMR_CC2S_MASK,
              CH32H41X_TIM_CCMR_CC1S_IN_TI1 |
              CH32H41X_TIM_CCMR_CC2S_IN_TI1);

  /* CC1P = 0 (rising), CC2P = 1 (falling); both captures enabled */

  modifyreg32(CH32H41X_TIM_CCER(base),
              CH32H41X_TIM_CCE_CC1P | CH32H41X_TIM_CCE_CC2P |
              CH32H41X_TIM_CCE_CC1E | CH32H41X_TIM_CCE_CC2E,
              CH32H41X_TIM_CCE_CC1E | CH32H41X_TIM_CCE_CC2E |
              CH32H41X_TIM_CCE_CC2P);

  /* Input filter: 8 clock cycles of glitch rejection */

  modifyreg32(CH32H41X_TIM_CHCTLR1(base),
              CH32H41X_TIM_CCMR_IC1F_MASK,
              (0x3ul << CH32H41X_TIM_CCMR_IC1F_SHIFT));

  /* Slave mode: reset counter on rising TI1 edge (SMS=100, TS=101) */

  modifyreg32(CH32H41X_TIM_SMCFGR(base),
              CH32H41X_TIM_SM_SMS_MASK | CH32H41X_TIM_SM_TS_MASK,
              (0x4ul << CH32H41X_TIM_SM_SMS_SHIFT) |
              (0x5ul << CH32H41X_TIM_SM_TS_SHIFT));

  /* Enable the capture interrupts */

  modifyreg32(CH32H41X_TIM_DMAINTENR(base), 0,
              CH32H41X_TIM_DIER_CC1IE | CH32H41X_TIM_DIER_CC2IE);
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
              CH32H41X_TIM_DIER_CC1IE | CH32H41X_TIM_DIER_CC2IE, 0);
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
