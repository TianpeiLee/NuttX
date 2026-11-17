/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_pwm.c
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
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/timers/pwm.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_tim.h"

#include "ch32h41x_internal.h"

#ifdef CONFIG_CH32H41X_PWM

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TIM1_IDX   0
#define TIM2_IDX   1
#define TIM3_IDX   2
#define TIM4_IDX   3
#define TIM5_IDX   4
#define TIM6_IDX   5
#define TIM7_IDX   6
#define TIM8_IDX   7
#define TIM9_IDX   8
#define TIM10_IDX  9
#define TIM11_IDX 10
#define TIM12_IDX 11

/* TIM9..TIM12 (indexes 8..11) are 32-bit timers on this family: CNT,
 * ATRLR and the CHxCVR registers are 32 bits wide, so ARR/CCR must be
 * written as full 32-bit values.  The 16-bit half-word accesses leave
 * the upper bits untouched and would corrupt the period. */

static bool ch32h41x_pwm_is32bit(int timer)
{
  return timer >= TIM9_IDX;
}

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_pwmtimer_s
{
  struct pwm_lowerhalf_s lower;  /* must be first */
  uint8_t timer;                 /* 0 = TIM1 .. 4 = TIM5 */
  uint8_t channels;              /* number of channels (4) */
  bool running;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int ch32h41x_pwm_setup(FAR struct pwm_lowerhalf_s *dev);
static int ch32h41x_pwm_shutdown(FAR struct pwm_lowerhalf_s *dev);
#ifdef CONFIG_PWM_PULSECOUNT
static int ch32h41x_pwm_start(FAR struct pwm_lowerhalf_s *dev,
                              FAR const struct pwm_info_s *info,
                              FAR void *handle);
#else
static int ch32h41x_pwm_start(FAR struct pwm_lowerhalf_s *dev,
                              FAR const struct pwm_info_s *info);
#endif
static int ch32h41x_pwm_stop(FAR struct pwm_lowerhalf_s *dev);
static int ch32h41x_pwm_ioctl(FAR struct pwm_lowerhalf_s *dev, int cmd,
                              unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct pwm_ops_s g_pwm_ops =
{
  .setup    = ch32h41x_pwm_setup,
  .shutdown = ch32h41x_pwm_shutdown,
  .start    = ch32h41x_pwm_start,
  .stop     = ch32h41x_pwm_stop,
  .ioctl    = ch32h41x_pwm_ioctl,
};

/* One PWM device per timer, allocated statically */

static struct ch32h41x_pwmtimer_s g_pwmtimers[] =
{
#ifdef CONFIG_CH32H41X_TIM1_PWM
  { .timer = TIM1_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM2_PWM
  { .timer = TIM2_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM3_PWM
  { .timer = TIM3_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM4_PWM
  { .timer = TIM4_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM5_PWM
  { .timer = TIM5_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM8_PWM
  { .timer = TIM8_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM9_PWM
  { .timer = TIM9_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM10_PWM
  { .timer = TIM10_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM11_PWM
  { .timer = TIM11_IDX, .channels = 4, .running = false },
#endif
#ifdef CONFIG_CH32H41X_TIM12_PWM
  { .timer = TIM12_IDX, .channels = 4, .running = false },
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_pwm_timerbase
 ****************************************************************************/

static uintptr_t ch32h41x_pwm_timerbase(uint8_t timer)
{
  static const uintptr_t bases[12] =
  {
    CH32H41X_TIM1_BASE, CH32H41X_TIM2_BASE, CH32H41X_TIM3_BASE,
    CH32H41X_TIM4_BASE, CH32H41X_TIM5_BASE, CH32H41X_TIM6_BASE,
    CH32H41X_TIM7_BASE, CH32H41X_TIM8_BASE, CH32H41X_TIM9_BASE,
    CH32H41X_TIM10_BASE, CH32H41X_TIM11_BASE, CH32H41X_TIM12_BASE,
  };

  return bases[timer];
}

/****************************************************************************
 * Name: ch32h41x_pwm_ccer
 ****************************************************************************/

static uint32_t ch32h41x_pwm_ccer(int channel)
{
  static const uint32_t ccer[4] =
  {
    CH32H41X_TIM_CCE_CC1E, CH32H41X_TIM_CCE_CC2E,
    CH32H41X_TIM_CCE_CC3E, CH32H41X_TIM_CCE_CC4E,
  };

  return ccer[channel];
}

/****************************************************************************
 * Name: ch32h41x_pwm_ocm_enable
 *
 * Description:
 *   Configure one channel as PWM1 output with preload and enable the
 *   compare output.
 *
 ****************************************************************************/

static void ch32h41x_pwm_ocm_enable(uintptr_t base, int channel)
{
  uintptr_t ccmr = (channel < 2) ? CH32H41X_TIM_CHCTLR1(base)
                                 : CH32H41X_TIM_CHCTLR2(base);
  int half = channel & 1;
  uint32_t ocm_shift = (half == 0) ? CH32H41X_TIM_CCMR_OC1M_SHIFT
                                   : CH32H41X_TIM_CCMR_OC2M_SHIFT;
  uint32_t ocpe = (half == 0) ? CH32H41X_TIM_CCMR_OC1PE
                              : CH32H41X_TIM_CCMR_OC2PE;
  uint32_t ccs_mask = (half == 0) ? CH32H41X_TIM_CCMR_CC1S_MASK
                                  : CH32H41X_TIM_CCMR_CC2S_MASK;

  /* Channel as output, PWM1 mode, preload enabled */

  modifyreg32(ccmr, ccs_mask | (7ul << ocm_shift) | ocpe,
              (0x6ul << ocm_shift) | ocpe);
  modifyreg32(CH32H41X_TIM_CCER(base), 0, ch32h41x_pwm_ccer(channel));
}

/****************************************************************************
 * Name: ch32h41x_pwm_setccr
 ****************************************************************************/

static void ch32h41x_pwm_setccr(uintptr_t base, int timer, int channel,
                                uint32_t value)
{
  uintptr_t addr = base + 0x34 + channel * 4;

  if (ch32h41x_pwm_is32bit(timer))
    {
      putreg32(value, addr);
    }
  else
    {
      putreg16((uint16_t)value, addr);
    }
}

/****************************************************************************
 * Name: ch32h41x_pwm_setup
 ****************************************************************************/

static int ch32h41x_pwm_setup(FAR struct pwm_lowerhalf_s *dev)
{
  FAR struct ch32h41x_pwmtimer_s *priv =
    (FAR struct ch32h41x_pwmtimer_s *)dev;

  ch32h41x_tim_clock_enable(priv->timer, true);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_pwm_shutdown
 ****************************************************************************/

static int ch32h41x_pwm_shutdown(FAR struct pwm_lowerhalf_s *dev)
{
  FAR struct ch32h41x_pwmtimer_s *priv =
    (FAR struct ch32h41x_pwmtimer_s *)dev;

  ch32h41x_pwm_stop(dev);
  ch32h41x_tim_clock_enable(priv->timer, false);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_pwm_start
 ****************************************************************************/

#ifdef CONFIG_PWM_PULSECOUNT
static int ch32h41x_pwm_start(FAR struct pwm_lowerhalf_s *dev,
                              FAR const struct pwm_info_s *info,
                              FAR void *handle)
#else
static int ch32h41x_pwm_start(FAR struct pwm_lowerhalf_s *dev,
                              FAR const struct pwm_info_s *info)
#endif
{
  FAR struct ch32h41x_pwmtimer_s *priv =
    (FAR struct ch32h41x_pwmtimer_s *)dev;
  uintptr_t base = ch32h41x_pwm_timerbase(priv->timer);
  uint32_t timclk;
  uint32_t div;
  uint32_t psc;
  uint32_t arr;
  uint32_t period;
  int ch;
#ifdef CONFIG_PWM_PULSECOUNT
  UNUSED(handle);
#endif

  if (info->frequency == 0)
    {
      return -EINVAL;
    }

  timclk = ch32h41x_hclk_frequency();
  div = timclk / info->frequency;
  if (div == 0)
    {
      return -EINVAL;
    }

  /* Choose the smallest prescaler keeping the reload in 16 bits */

  psc = div / 0x10000;
  arr = (div / (psc + 1));
  if (arr < 2)
    {
      return -EINVAL;
    }

  period = arr;                       /* counts per output period */
  arr -= 1;

  putreg16((uint16_t)psc, CH32H41X_TIM_PSC(base));
  if (ch32h41x_pwm_is32bit(priv->timer))
    {
      putreg32(arr, CH32H41X_TIM_ATRLR(base));
    }
  else
    {
      putreg16((uint16_t)arr, CH32H41X_TIM_ATRLR(base));
    }

  /* Duty cycles: ub16 fraction of the period.  The requested hardware
   * channel number (1-based, from info->channels[ch].channel) selects the
   * CCR/CCER/CHCTLR fields; unspecified slots are skipped so that other
   * channels (e.g. an input capture on CH1) are not disturbed. */

  for (ch = 0; ch < priv->channels; ch++)
    {
      uint32_t duty = info->channels[ch].duty;
      int hwch = (int)info->channels[ch].channel - 1;

      if (hwch < 0 || hwch >= priv->channels)
        {
          continue;      /* channel not present on this timer */
        }
      uint32_t ccr;

      if (hwch < 0 || hwch > 3)
        {
          continue;
        }

      ccr = ((uint64_t)period * duty + 0x8000ul) / 0x10000ul;

      if (ccr > period)
        {
          ccr = period;
        }

      ch32h41x_pwm_setccr(base, priv->timer, hwch, ccr);
      ch32h41x_pwm_ocm_enable(base, hwch);
    }

  /* Force an update so PSC/ATRLR/CCR take effect at once */

  putreg32(CH32H41X_TIM_EVG_UG, CH32H41X_TIM_SWEVGR(base));

  /* Enable the counter; advanced timers also need the main output (MOE) */

  if (priv->timer == TIM1_IDX || priv->timer == TIM8_IDX)
    {
      modifyreg32(CH32H41X_TIM_BDTR(base), 0,
                  CH32H41X_TIM_BD_MOE | CH32H41X_TIM_BD_AOE);
    }

  modifyreg32(CH32H41X_TIM_CTLR1(base), 0, CH32H41X_TIM_CR1_CEN);

  priv->running = true;
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_pwm_stop
 ****************************************************************************/

static int ch32h41x_pwm_stop(FAR struct pwm_lowerhalf_s *dev)
{
  FAR struct ch32h41x_pwmtimer_s *priv =
    (FAR struct ch32h41x_pwmtimer_s *)dev;
  uintptr_t base = ch32h41x_pwm_timerbase(priv->timer);
  int ch;

  modifyreg32(CH32H41X_TIM_CTLR1(base), CH32H41X_TIM_CR1_CEN, 0);

  for (ch = 0; ch < priv->channels; ch++)
    {
      modifyreg32(CH32H41X_TIM_CCER(base), ch32h41x_pwm_ccer(ch), 0);
    }

  if (priv->timer == TIM1_IDX || priv->timer == TIM8_IDX)
    {
      modifyreg32(CH32H41X_TIM_BDTR(base),
                  CH32H41X_TIM_BD_MOE | CH32H41X_TIM_BD_AOE, 0);
    }

  priv->running = false;
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_pwm_ioctl
 ****************************************************************************/

static int ch32h41x_pwm_ioctl(FAR struct pwm_lowerhalf_s *dev, int cmd,
                              unsigned long arg)
{
  /* Platform-specific commands (dead-time configuration for TIM1) can be
   * added here when the ESC-oriented PX4 layer needs them. */

  return -ENOTTY;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_pwm_initialize
 *
 * Description:
 *   Register /dev/pwm0.. for every enabled timer PWM device.
 *
 ****************************************************************************/

int ch32h41x_pwm_initialize(void)
{
  int minor;
  int i;
  int ret = OK;

  minor = 0;
  for (i = 0; i < (int)sizeof(g_pwmtimers) / sizeof(g_pwmtimers[0]); i++)
    {
      FAR struct ch32h41x_pwmtimer_s *priv = &g_pwmtimers[i];
      char path[16];

      priv->lower.ops = &g_pwm_ops;

      snprintf(path, sizeof(path), "/dev/pwm%d", minor);
      ret = pwm_register(path, &priv->lower);
      if (ret < 0)
        {
          return ret;
        }

      minor++;
    }

  return OK;
}

#endif /* CONFIG_CH32H41X_PWM */
