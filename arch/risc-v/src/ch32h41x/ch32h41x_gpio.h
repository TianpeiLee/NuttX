/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_gpio.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_CH32H41X_GPIO_H
#define __ARCH_RISCV_SRC_CH32H41X_CH32H41X_GPIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>

/* GPIO port indices used by the board pin descriptions */

#define GPIO_PORT_A   0
#define GPIO_PORT_B   1
#define GPIO_PORT_C   2
#define GPIO_PORT_D   3
#define GPIO_PORT_E   4
#define GPIO_PORT_F   5

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_gpioinit
 *
 * Description:
 *   Enable the AFIO and all GPIO port clocks.  Called from the board
 *   initialization.
 *
 ****************************************************************************/

void ch32h41x_gpioinit(void);

/****************************************************************************
 * Name: ch32h41x_gpio_setmode
 *
 * Description:
 *   Configure the mode nibble of a pin (see CH32H41X_GPIO_MODE_* values in
 *   hardware/ch32h41x_gpio.h).
 *
 *   Input Parameters:
 *     port - GPIO port index (GPIO_PORT_A .. GPIO_PORT_F)
 *     pin  - pin number 0..15
 *     mode - mode nibble (MODE[1:0] | CNF[1:0])
 *
 ****************************************************************************/

void ch32h41x_gpio_setmode(uint8_t port, uint8_t pin, uint8_t mode);

/****************************************************************************
 * Name: ch32h41x_gpio_setaf
 *
 * Description:
 *   Select the alternate function for a pin.
 *
 ****************************************************************************/

void ch32h41x_gpio_setaf(uint8_t port, uint8_t pin, uint8_t af);

/****************************************************************************
 * Name: ch32h41x_gpio_write / ch32h41x_gpio_read
 *
 * Description:
 *   Set/read a GPIO pin level.  For inputs configured as pull-up/down the
 *   pull direction follows the current output register bit.
 *
 ****************************************************************************/

void ch32h41x_gpio_write(uint8_t port, uint8_t pin, bool value);
bool ch32h41x_gpio_read(uint8_t port, uint8_t pin);

#endif /* __ARCH_RISCV_SRC_CH32H41X_CH32H41X_GPIO_H */
