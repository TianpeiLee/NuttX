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
#include <arch/board/board.h>

#include "ch32h41xevb.h"
#include "ch32h41x_internal.h"

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

  ret = ch32h41x_gpio_register(0, 4, GPIO_INTERRUPT_BOTH_PIN, 2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "gpio2 register failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_CH32H41X_HAVE_TIM1_PWM
  ret = ch32h41x_pwm_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "pwm register failed: %d\n", ret);
    }

  /* PWM self-test output pin: PE11 = TIM1_CH2 */

  ch32h41x_gpio_setmode(GPIO_TIM1_CH2_PORT, GPIO_TIM1_CH2_PIN,
                        CH32H41X_GPIO_MODE_AF_PP_50);
  ch32h41x_gpio_setaf(GPIO_TIM1_CH2_PORT, GPIO_TIM1_CH2_PIN,
                      GPIO_TIM1_CH2_AF);
#endif

#ifdef CONFIG_CH32H41X_HAVE_TIM2_CAPTURE
  ret = ch32h41x_capture_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "capture register failed: %d\n", ret);
    }

  /* Capture self-test input pin: PA0 = TIM2_CH1 */

  ch32h41x_gpio_setaf(GPIO_TIM2_CH1_PORT, GPIO_TIM2_CH1_PIN,
                      GPIO_TIM2_CH1_AF);
  ch32h41x_gpio_setmode(GPIO_TIM2_CH1_PORT, GPIO_TIM2_CH1_PIN,
                        CH32H41X_GPIO_MODE_IN_FLOAT);
#endif

#ifdef CONFIG_CH32H41X_IWDG
  ret = ch32h41x_iwdg_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "watchdog0 register failed: %d\n", ret);
    }
#endif

#endif /* CONFIG_NSH_ARCHINIT */

  return OK;
}
