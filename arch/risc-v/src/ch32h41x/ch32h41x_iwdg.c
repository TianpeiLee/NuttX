/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_iwdg.c
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
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/timers/watchdog.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_iwdg.h"

#include "ch32h41x_internal.h"

#ifdef CONFIG_CH32H41X_IWDG

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The independent watchdog runs from the 40kHz LSI.  The prescaler divides
 * it by 4*2^n and the reload counter is 12 bits (0..0xfff).  Maximum
 * timeout: 4096 * 256 / 40000 s = 26.2 s.  Minimum: 4 / 40000 = 0.1 ms.
 */

#define IWDG_LSI_CLOCK       40000
#define IWDG_MAX_PRESCALER   256
#define IWDG_MAX_RELOAD      0xfff
#define IWDG_MAX_TIMEOUT_MSEC \
  ((IWDG_MAX_RELOAD + 1) * 1000 * IWDG_MAX_PRESCALER / IWDG_LSI_CLOCK)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_iwdg_s
{
  struct watchdog_lowerhalf_s lower;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int ch32h41x_iwdg_start(FAR struct watchdog_lowerhalf_s *lower);
static int ch32h41x_iwdg_stop(FAR struct watchdog_lowerhalf_s *lower);
static int ch32h41x_iwdg_keepalive(FAR struct watchdog_lowerhalf_s *lower);
static int ch32h41x_iwdg_getstatus(FAR struct watchdog_lowerhalf_s *lower,
                                   FAR struct watchdog_status_s *status);
static int ch32h41x_iwdg_settimeout(FAR struct watchdog_lowerhalf_s *lower,
                                    uint32_t timeout);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct watchdog_ops_s g_iwdg_ops =
{
  .start      = ch32h41x_iwdg_start,
  .stop       = ch32h41x_iwdg_stop,
  .keepalive  = ch32h41x_iwdg_keepalive,
  .getstatus  = ch32h41x_iwdg_getstatus,
  .settimeout = ch32h41x_iwdg_settimeout,
  .capture    = NULL,
  .ioctl      = NULL,
};

static struct ch32h41x_iwdg_s g_iwdg;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_iwdg_start
 *
 * Description:
 *   Start the watchdog.  The IWDG cannot be stopped once started; the
 *   stop() operation therefore only disarms the API, the hardware keeps
 *   running (per the independent-watchdog contract).
 *
 ****************************************************************************/

static int ch32h41x_iwdg_start(FAR struct watchdog_lowerhalf_s *lower)
{
  putreg32(CH32H41X_IWDG_KEY_START, CH32H41X_IWDG_CTLR);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_iwdg_stop
 ****************************************************************************/

static int ch32h41x_iwdg_stop(FAR struct watchdog_lowerhalf_s *lower)
{
  /* The independent watchdog cannot be stopped by software */

  return -EIO;
}

/****************************************************************************
 * Name: ch32h41x_iwdg_keepalive
 ****************************************************************************/

static int ch32h41x_iwdg_keepalive(FAR struct watchdog_lowerhalf_s *lower)
{
  putreg32(CH32H41X_IWDG_KEY_RELOAD, CH32H41X_IWDG_CTLR);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_iwdg_getstatus
 ****************************************************************************/

static int ch32h41x_iwdg_getstatus(FAR struct watchdog_lowerhalf_s *lower,
                                   FAR struct watchdog_status_s *status)
{
  uint32_t pscr = getreg32(CH32H41X_IWDG_PSCR) & CH32H41X_IWDG_PSC_MASK;
  uint32_t rldr = getreg32(CH32H41X_IWDG_RLDR);

  status->flags    = WDFLAGS_RESET;
  status->timeout  = (rldr + 1) * 1000 * (4ul << pscr) / IWDG_LSI_CLOCK;
  status->timeleft = 0;          /* not readable on this block */
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_iwdg_settimeout
 *
 * Description:
 *   Select the smallest prescaler that covers the requested timeout and
 *   program the reload counter.  The write-access key is required before
 *   touching PSCR/RLDR.
 *
 ****************************************************************************/

static int ch32h41x_iwdg_settimeout(FAR struct watchdog_lowerhalf_s *lower,
                                    uint32_t timeout)
{
  uint32_t prescaler;
  uint32_t reload;
  uint32_t reg;

  if (timeout == 0 || timeout > IWDG_MAX_TIMEOUT_MSEC)
    {
      return -EINVAL;
    }

  /* prescaler * (reload+1) / 40000 = timeout_ms / 1000;  pick the smallest
   * prescaler so that the reload value stays within 12 bits. */

  for (prescaler = 4; prescaler < IWDG_MAX_PRESCALER; prescaler <<= 1)
    {
      reload = ((uint64_t)timeout * IWDG_LSI_CLOCK) / (prescaler * 1000);
      if (reload <= IWDG_MAX_RELOAD)
        {
          break;
        }
    }

  if (reload > IWDG_MAX_RELOAD)
    {
      reload = IWDG_MAX_RELOAD;
    }

  putreg32(CH32H41X_IWDG_KEY_WRITE, CH32H41X_IWDG_CTLR);

  reg = getreg32(CH32H41X_IWDG_PSCR);
  reg &= ~CH32H41X_IWDG_PSC_MASK;
  reg |= prescaler >> 2;                 /* bit pattern: 4*2^n -> n */
  putreg32(reg, CH32H41X_IWDG_PSCR);

  putreg32(reload, CH32H41X_IWDG_RLDR);

  /* Reload so the new timeout applies immediately */

  putreg32(CH32H41X_IWDG_KEY_RELOAD, CH32H41X_IWDG_CTLR);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_iwdg_initialize
 *
 * Description:
 *   Register the independent watchdog as /dev/watchdog0.  The IWDG runs
 *   from its own LSI clock; no bus clock enable is required.
 *
 ****************************************************************************/

int ch32h41x_iwdg_initialize(void)
{
  FAR void *handle;

  g_iwdg.lower.ops = &g_iwdg_ops;

  handle = watchdog_register("/dev/watchdog0", &g_iwdg.lower);
  if (handle == NULL)
    {
      return -ENOMEM;
    }

  return OK;
}

#endif /* CONFIG_CH32H41X_IWDG */
