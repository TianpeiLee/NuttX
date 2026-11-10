/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_internal.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_CH32H41X_INTERNAL_H
#define __ARCH_RISCV_SRC_CH32H41X_CH32H41X_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/irq.h>
#ifdef CONFIG_DEV_GPIO
#  include <nuttx/ioexpander/gpio.h>
#endif

/* Provided by ch32h41x_clockconfig.c */

void ch32h41x_clockconfig(void);
uint32_t ch32h41x_sysclk_frequency(void);
uint32_t ch32h41x_hclk_frequency(void);

/* Provided by ch32h41x_lowputc.c */

void ch32h41x_lowsetup(void);
void riscv_lowputc(char ch);

/* Provided by ch32h41x_gpio.c */

#include "hardware/ch32h41x_gpio.h"

void ch32h41x_gpioinit(void);
void ch32h41x_gpio_setmode(uint8_t port, uint8_t pin, uint8_t mode);
void ch32h41x_gpio_setaf(uint8_t port, uint8_t pin, uint8_t af);
void ch32h41x_gpio_write(uint8_t port, uint8_t pin, bool value);
bool ch32h41x_gpio_read(uint8_t port, uint8_t pin);

#ifdef CONFIG_DEV_GPIO
int ch32h41x_gpio_register(uint8_t port, uint8_t pin,
                           enum gpio_pintype_e pintype, int minor);
#endif

/* Provided by ch32h41x_exti.c */

#ifdef CONFIG_CH32H41X_EXTI
typedef int (*ch32h41x_exti_callback_t)(void *arg);
void ch32h41x_exti_initialize(void);
int ch32h41x_exti_attach(uint8_t port, uint8_t pin, uint8_t trigger,
                         ch32h41x_exti_callback_t callback, void *cbarg);
void ch32h41x_exti_enable(uint8_t pin, bool enable);
#endif

/* Provided by ch32h41x_iwdg.c */

#ifdef CONFIG_CH32H41X_IWDG
int ch32h41x_iwdg_initialize(void);
#endif

/* Provided by ch32h41x_tim.c */

#ifdef CONFIG_CH32H41X_TIM
typedef int (*timisr_t)(int irq, void *context, void *arg);
void ch32h41x_tim_clock_enable(int timer, bool enable);
int ch32h41x_tim_irqattach(int timer, int irqsel, timisr_t isr);
#endif

/* Provided by ch32h41x_pwm.c */

#ifdef CONFIG_CH32H41X_PWM
int ch32h41x_pwm_initialize(void);
#endif

/* Provided by ch32h41x_capture.c */

#ifdef CONFIG_CH32H41X_TIM2_CAPTURE
int ch32h41x_capture_initialize(void);
#endif

#endif /* __ARCH_RISCV_SRC_CH32H41X_CH32H41X_INTERNAL_H */
