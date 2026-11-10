/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_gpio.c
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
#include <stdbool.h>
#include <assert.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_gpio.h"

#include "ch32h41x_gpio.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uintptr_t g_gpio_base[CH32H41X_GPIO_NPORTS] =
{
  CH32H41X_GPIOA_BASE, CH32H41X_GPIOB_BASE, CH32H41X_GPIOC_BASE,
  CH32H41X_GPIOD_BASE, CH32H41X_GPIOE_BASE, CH32H41X_GPIOF_BASE,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uintptr_t ch32h41x_gpio_port(uint8_t port)
{
  DEBUGASSERT(port < CH32H41X_GPIO_NPORTS);
  return g_gpio_base[port];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_gpioinit
 ****************************************************************************/

void ch32h41x_gpioinit(void)
{
  /* Enable the AFIO and GPIOA..GPIOF clocks on HB2 */

  modifyreg32(CH32H41X_RCC_HB2PCENR, 0,
              CH32H41X_RCC_HB2_AFIO |
              CH32H41X_RCC_HB2_GPIOA | CH32H41X_RCC_HB2_GPIOB |
              CH32H41X_RCC_HB2_GPIOC | CH32H41X_RCC_HB2_GPIOD |
              CH32H41X_RCC_HB2_GPIOE | CH32H41X_RCC_HB2_GPIOF);
}

/****************************************************************************
 * Name: ch32h41x_gpio_setmode
 ****************************************************************************/

void ch32h41x_gpio_setmode(uint8_t port, uint8_t pin, uint8_t mode)
{
  uintptr_t base = ch32h41x_gpio_port(port);
  uintptr_t reg = base + CH32H41X_GPIO_CFGLR_REG(pin);
  uint32_t shift = CH32H41X_GPIO_MODE_SHIFT(pin);

  modifyreg32(reg, CH32H41X_GPIO_MODE_MASK(pin),
              (uint32_t)mode << shift);

  /* For pull-up/pull-down inputs the pull direction is taken from the
   * output data register; make sure the preconfigured output value is
   * visible before the input mode is entered (done by the caller via
   * ch32h41x_gpio_write()).
   */
}

/****************************************************************************
 * Name: ch32h41x_gpio_setaf
 ****************************************************************************/

void ch32h41x_gpio_setaf(uint8_t port, uint8_t pin, uint8_t af)
{
  uintptr_t reg;

  if (pin < 8)
    {
      reg = CH32H41X_AFIO_AFLR(port);
    }
  else
    {
      reg = CH32H41X_AFIO_AFHR(port);
    }

  modifyreg32(reg, CH32H41X_AFIO_AF_MASK(pin),
              (uint32_t)af << CH32H41X_AFIO_AF_SHIFT(pin));
}

/****************************************************************************
 * Name: ch32h41x_gpio_write
 ****************************************************************************/

void ch32h41x_gpio_write(uint8_t port, uint8_t pin, bool value)
{
  uintptr_t base = ch32h41x_gpio_port(port);

  if (value)
    {
      putreg32(CH32H41X_GPIO_BSHR_SET(pin), base + CH32H41X_GPIO_BSHR);
    }
  else
    {
      putreg32(CH32H41X_GPIO_BSHR_RESET(pin), base + CH32H41X_GPIO_BSHR);
    }
}

/****************************************************************************
 * Name: ch32h41x_gpio_read
 ****************************************************************************/

bool ch32h41x_gpio_read(uint8_t port, uint8_t pin)
{
  uintptr_t base = ch32h41x_gpio_port(port);

  return (getreg32(base + CH32H41X_GPIO_INDR) & (1ul << pin)) != 0;
}
