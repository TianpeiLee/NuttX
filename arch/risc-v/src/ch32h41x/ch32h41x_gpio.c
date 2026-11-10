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
#include <errno.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>
#include <nuttx/ioexpander/gpio.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_gpio.h"
#include "hardware/ch32h41x_exti.h"

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

/****************************************************************************
 * GPIO character device (lower half).  Registered by the board bring-up
 * for the pins listed in the board gpio table; exposes /dev/gpioN.
 ****************************************************************************/

#ifdef CONFIG_DEV_GPIO

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_gpiodev_s
{
  struct gpio_dev_s dev;        /* exported interface */
  uint8_t port;                 /* GPIO port index */
  uint8_t pin;                  /* pin number */
  pin_interrupt_t callback;     /* interrupt callback */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int ch32h41x_gpin_read(FAR struct gpio_dev_s *dev, FAR bool *value);
static int ch32h41x_gpio_write_dev(FAR struct gpio_dev_s *dev, bool value);
static int ch32h41x_gpio_attach(FAR struct gpio_dev_s *dev,
                                pin_interrupt_t callback);
static int ch32h41x_gpio_enable(FAR struct gpio_dev_s *dev, bool enable);
static int ch32h41x_gpio_setpintype(FAR struct gpio_dev_s *dev,
                                    enum gpio_pintype_e pintype);
static int ch32h41x_gpio_isr(void *arg);

static const struct gpio_operations_s g_gpio_in_ops =
{
  .go_read       = ch32h41x_gpin_read,
  .go_write      = NULL,
  .go_attach     = NULL,
  .go_enable     = NULL,
  .go_setpintype = ch32h41x_gpio_setpintype,
};

static const struct gpio_operations_s g_gpio_out_ops =
{
  .go_read       = ch32h41x_gpin_read,
  .go_write      = ch32h41x_gpio_write_dev,
  .go_attach     = NULL,
  .go_enable     = NULL,
  .go_setpintype = ch32h41x_gpio_setpintype,
};

static const struct gpio_operations_s g_gpio_int_ops =
{
  .go_read       = ch32h41x_gpin_read,
  .go_write      = NULL,
  .go_attach     = ch32h41x_gpio_attach,
  .go_enable     = ch32h41x_gpio_enable,
  .go_setpintype = ch32h41x_gpio_setpintype,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_gpin_read
 ****************************************************************************/

static int ch32h41x_gpin_read(FAR struct gpio_dev_s *dev, FAR bool *value)
{
  FAR struct ch32h41x_gpiodev_s *priv =
    (FAR struct ch32h41x_gpiodev_s *)dev;

  *value = ch32h41x_gpio_read(priv->port, priv->pin);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_gpio_write_dev
 ****************************************************************************/

static int ch32h41x_gpio_write_dev(FAR struct gpio_dev_s *dev, bool value)
{
  FAR struct ch32h41x_gpiodev_s *priv =
    (FAR struct ch32h41x_gpiodev_s *)dev;

  ch32h41x_gpio_write(priv->port, priv->pin, value);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_gpio_attach
 ****************************************************************************/

static int ch32h41x_gpio_attach(FAR struct gpio_dev_s *dev,
                                pin_interrupt_t callback)
{
  FAR struct ch32h41x_gpiodev_s *priv =
    (FAR struct ch32h41x_gpiodev_s *)dev;

  priv->callback = callback;
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_gpio_enable
 ****************************************************************************/

static int ch32h41x_gpio_enable(FAR struct gpio_dev_s *dev, bool enable)
{
  FAR struct ch32h41x_gpiodev_s *priv =
    (FAR struct ch32h41x_gpiodev_s *)dev;

  ch32h41x_exti_enable(priv->pin, enable);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_gpio_setpintype
 *
 * Description:
 *   The example gpio application reconfigures its pins through this
 *   operation when a different mode is requested via the -t option.
 *
 ****************************************************************************/

static int ch32h41x_gpio_setpintype(FAR struct gpio_dev_s *dev,
                                    enum gpio_pintype_e pintype)
{
  FAR struct ch32h41x_gpiodev_s *priv =
    (FAR struct ch32h41x_gpiodev_s *)dev;
  uint8_t trigger;

  switch (pintype)
    {
      case GPIO_INPUT_PIN:
        ch32h41x_gpio_setmode(priv->port, priv->pin,
                              CH32H41X_GPIO_MODE_IN_FLOAT);
        break;

      case GPIO_INPUT_PIN_PULLUP:
        ch32h41x_gpio_write(priv->port, priv->pin, true);
        ch32h41x_gpio_setmode(priv->port, priv->pin,
                              CH32H41X_GPIO_MODE_IN_PULL);
        break;

      case GPIO_INPUT_PIN_PULLDOWN:
        ch32h41x_gpio_write(priv->port, priv->pin, false);
        ch32h41x_gpio_setmode(priv->port, priv->pin,
                              CH32H41X_GPIO_MODE_IN_PULL);
        break;

      case GPIO_OUTPUT_PIN:
        ch32h41x_gpio_setmode(priv->port, priv->pin,
                              CH32H41X_GPIO_MODE_OUT_PP_50);
        break;

      case GPIO_OUTPUT_PIN_OPENDRAIN:
        ch32h41x_gpio_setmode(priv->port, priv->pin,
                              CH32H41X_GPIO_MODE_OUT_OD_50);
        break;

      case GPIO_INTERRUPT_RISING_PIN:
      case GPIO_INTERRUPT_FALLING_PIN:
      case GPIO_INTERRUPT_BOTH_PIN:
        ch32h41x_gpio_setmode(priv->port, priv->pin,
                              CH32H41X_GPIO_MODE_IN_FLOAT);

        trigger = (pintype == GPIO_INTERRUPT_RISING_PIN) ? 0 :
                  (pintype == GPIO_INTERRUPT_FALLING_PIN) ? 1 : 2;
        ch32h41x_exti_attach(priv->port, priv->pin, trigger,
                             ch32h41x_gpio_isr, dev);
        break;

      default:
        return -EINVAL;
    }

  dev->gp_pintype = pintype;
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_gpio_isr
 *
 * Description:
 *   EXTI callback bridge: forward to the attached user callback.
 *
 ****************************************************************************/

static int ch32h41x_gpio_isr(void *arg)
{
  FAR struct ch32h41x_gpiodev_s *priv = (FAR struct ch32h41x_gpiodev_s *)arg;

  if (priv->callback != NULL)
    {
      priv->callback(&priv->dev, priv->pin);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_gpio_register
 *
 * Description:
 *   Create the GPIO character device /dev/gpio<minor> for the given pin.
 *   The initial pin type selects the hardware mode and the operation
 *   vector.
 *
 ****************************************************************************/

int ch32h41x_gpio_register(uint8_t port, uint8_t pin,
                           enum gpio_pintype_e pintype, int minor)
{
  FAR struct ch32h41x_gpiodev_s *priv;
  int ret;

  if (port >= CH32H41X_GPIO_NPORTS || pin > 15 || minor < 0)
    {
      return -EINVAL;
    }

  priv = kmm_zalloc(sizeof(struct ch32h41x_gpiodev_s));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  priv->port     = port;
  priv->pin      = pin;
  priv->dev.gp_pintype = pintype;
  priv->callback = NULL;

  /* Apply the hardware mode matching the requested pin type */

  ret = ch32h41x_gpio_setpintype(&priv->dev, pintype);
  if (ret != OK)
    {
      kmm_free(priv);
      return ret;
    }

  switch (pintype)
    {
      case GPIO_INPUT_PIN:
      case GPIO_INPUT_PIN_PULLUP:
      case GPIO_INPUT_PIN_PULLDOWN:
        priv->dev.gp_ops = &g_gpio_in_ops;
        break;

      case GPIO_OUTPUT_PIN:
      case GPIO_OUTPUT_PIN_OPENDRAIN:
        priv->dev.gp_ops = &g_gpio_out_ops;
        break;

      case GPIO_INTERRUPT_RISING_PIN:
      case GPIO_INTERRUPT_FALLING_PIN:
      case GPIO_INTERRUPT_BOTH_PIN:
        priv->dev.gp_ops = &g_gpio_int_ops;
        break;

      default:
        kmm_free(priv);
        return -EINVAL;
    }

  ret = gpio_pin_register(&priv->dev, minor);
  if (ret < 0)
    {
      kmm_free(priv);
    }

  return ret;
}

#endif /* CONFIG_DEV_GPIO */
