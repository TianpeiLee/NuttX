/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/src/ch32h41xevb_bringup.c
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

#include <sys/types.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/sched.h>
#include <arch/board/board.h>

#include "ch32h41xevb.h"
#include "ch32h41x_internal.h"

/****************************************************************************
 * TEMP DIAGNOSTIC: boot-stage heap audit
 *
 * After every bring-up stage, walk the heap allocation-node chain
 * (mm_allocnode_s: {preceding, size} with bit0 of size = allocated) and
 * record (stage id, node count, free bytes, first anomaly address) into
 * a fixed RAM block at 0x2017f000 that a debugger reads back.  This
 * localizes the boot-time heap corruption to the bring-up stage where
 * it first appears without needing a working console.
 ****************************************************************************/

#define HW_BLOCK      0x2017f000ul
#define HW_MAGIC      0x48574143ul  /* 'HWAC' */

static void hw_audit(int stage)
{
  extern uint8_t __heap_end[];
  uintptr_t gits;
  uintptr_t pos;
  uintptr_t end;
  unsigned nodes;
  unsigned freebytes;
  unsigned long firstsize;
  uintptr_t anomaly;

  /* heap start = MM_ALIGN_UP(g_idle_topstack value, 8) */

  {
    extern uintptr_t g_idle_topstack;

    gits = g_idle_topstack;
  }
  pos = (gits + 7) & ~7ul;
  end = (uintptr_t)__heap_end;
  nodes = 0;
  freebytes = 0;
  firstsize = 0;
  anomaly = 0;

  while (pos + 8 <= end)
    {
      unsigned long size = *(volatile unsigned long *)(pos + 4);
      unsigned long sz = size & ~1ul;

      if (sz < 8 || (sz & 7) != 0 || pos + sz > end)
        {
          anomaly = pos;
          break;
        }

      if (nodes == 0)
        {
          firstsize = size;
        }

      nodes++;
      if ((size & 1ul) == 0)
        {
          freebytes += sz;
        }

      pos += sz;
    }

  /* History slot per stage: {stage, gits, end, firstsize} */

  *(volatile unsigned long *)(HW_BLOCK + 20 + 16 * stage + 0) = (unsigned long)stage;
  *(volatile unsigned long *)(HW_BLOCK + 20 + 16 * stage + 4) = gits;
  *(volatile unsigned long *)(HW_BLOCK + 20 + 16 * stage + 8) = end;
  *(volatile unsigned long *)(HW_BLOCK + 20 + 16 * stage + 12) = firstsize;
}

#define HW_STAGE(s) hw_audit(s)

/****************************************************************************
 * Name: ledblink_task
 *
 * Description:
 *   Heartbeat task: toggles PA7 every 500 ms.  Gives a visible proof
 *   that the scheduler is alive even when no console output was seen:
 *   the NSH prompt is printed only once right after boot, so a terminal
 *   that attaches later shows nothing until a newline is sent.  If the
 *   LED blinks, the system is up - just press Enter in the terminal.
 *
 ****************************************************************************/

static int ledblink_task(int argc, FAR char *argv[])
{
  bool state = false;

  (void)argc;
  (void)argv;

  ch32h41x_gpio_setmode(0, 7, CH32H41X_GPIO_MODE_OUT_PP_50);
  ch32h41x_gpio_write(0, 7, state);

  for (; ; )
    {
      nxsig_usleep(500 * 1000);
      state = !state;
      ch32h41x_gpio_write(0, 7, state);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_app_initialize
 *
 * Description:
 *   Perform application specific initialization.  This function is never
 *   called directly from application code but by the logic during start
 *   up.  This function must be called *after* board_initialize().
 *
 ****************************************************************************/

int board_app_initialize(uintptr_t arg)
{
  int ret = OK;

#ifdef CONFIG_NSH_ARCHINIT

HW_STAGE(1);
#ifdef CONFIG_CH32H41X_EXTI
  /* Attach the shared EXTI vectors (interrupts are fully enabled at
   * this point) */

  ch32h41x_exti_initialize();
#endif

#ifdef CONFIG_DEV_GPIO
  /* Self-test GPIO devices for the gpio example:
   *   /dev/gpio0 PA7  output     (jumper to gpio1 input)
   *   /dev/gpio1 PA6  input
   *   /dev/gpio2 PA4  both-edge interrupt input
   */

  HW_STAGE(2);
  ret = ch32h41x_gpio_register(0, 7, GPIO_OUTPUT_PIN, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "gpio0 register failed: %d\n", ret);
    }

  ret = ch32h41x_gpio_register(0, 6, GPIO_INPUT_PIN, 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "gpio1 register failed: %d\n", ret);
    }

  HW_STAGE(3);
  ret = ch32h41x_gpio_register(0, 4, GPIO_INTERRUPT_BOTH_PIN, 2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "gpio2 register failed: %d\n", ret);
    }

  /* Define the pad levels of the input pins AFTER registration: the
   * register path reconfigures them as floating inputs, and a floating
   * EXTI input fires a continuous interrupt storm (every edge
   * preempts boot; the unexpected-IRQ / gpio ISR paths hammer the
   * console) so the board never reaches NSH.  For an F1-style pad
   * IN_PULL with ODR=0 selects the pull-down. */

  ch32h41x_gpio_setmode(0, 6, CH32H41X_GPIO_MODE_IN_PULL);
  ch32h41x_gpio_write(0, 6, false);

  ch32h41x_gpio_setmode(0, 4, CH32H41X_GPIO_MODE_IN_PULL);
  ch32h41x_gpio_write(0, 4, false);
HW_STAGE(5);
#endif


HW_STAGE(6);
#ifdef CONFIG_CH32H41X_HAVE_TIM1_PWM
  ret = ch32h41x_pwm_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "pwm register failed: %d\n", ret);
    }

  /* PWM self-test output pin: PA8 = TIM1_CH1 (DS0 AF table) */

  ch32h41x_gpio_setmode(GPIO_TIM1_CH1_PORT, GPIO_TIM1_CH1_PIN,
                        CH32H41X_GPIO_MODE_AF_PP_50);
  ch32h41x_gpio_setaf(GPIO_TIM1_CH1_PORT, GPIO_TIM1_CH1_PIN,
                      GPIO_TIM1_CH1_AF);
#endif

HW_STAGE(7);
#ifdef CONFIG_CH32H41X_HAVE_TIM2_CAPTURE
  ret = ch32h41x_capture_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "capture register failed: %d\n", ret);
    }

  /* Capture input pin: PA1 = TIM2_CH2 TI2 (DS0 AF table).  The AF
   * selection routes the pad into the timer input path; without it the
   * capture never sees edges even though the timer config is correct.
   */

  ch32h41x_gpio_setmode(GPIO_TIM2_CAP_PORT, GPIO_TIM2_CAP_PIN,
                        CH32H41X_GPIO_MODE_IN_FLOAT);
  ch32h41x_gpio_setaf(GPIO_TIM2_CAP_PORT, GPIO_TIM2_CAP_PIN,
                      GPIO_TIM2_CAP_AF);
#endif

HW_STAGE(4);
#ifdef CONFIG_CH32H41X_IWDG
  ret = ch32h41x_iwdg_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "watchdog0 register failed: %d\n", ret);
    }
#endif

#endif /* CONFIG_NSH_ARCHINIT */

  /* Start the PA7 heartbeat so scheduler liveness is visible without a
   * console (see ledblink_task). */

  ret = task_create("ledblink", 100, 1024, ledblink_task, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ledblink task create failed: %d\n", ret);
    }

  return OK;
}

/****************************************************************************
 * Name: board_late_initialize
 *
 * Description:
 *   Same driver bring-up as board_appinitialize(), but invoked from the
 *   kernel boot path (CONFIG_BOARD_LATE_INITIALIZE) so the peripherals
 *   are registered even when the init entrypoint is not NSH (headless
 *   self-test builds).
 *
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
int board_late_initialize(void)
{
  return board_app_initialize(0);
}
#endif
