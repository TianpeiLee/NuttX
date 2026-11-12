/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_adc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* ADC1 lower-half driver for /dev/adc0.
 *
 * The bring-up sequence follows the WCH EVT reference code exactly:
 *  - EXAM/ADC/ADC_DMA/Common/hardware.c (USBHS_PLL + ADC_Function_Init)
 *  - EXAM/ADC/Internal_Temperature/Common/hardware.c (internal channels
 *    need the output buffer enabled, CTLR1.BUFEN)
 * The ADC clock is taken from the USBHS PLL (480MHz): /36 while the
 * calibration runs, /6 afterwards (EVT ADC_SPEED_High path).  All
 * register bit positions in this file were cross-checked against the
 * EVT headers (ch32h417.h / ch32h417_adc.h), not against STM32 habits:
 *  - SAMPTR1/SAMPTR2 are SWAPPED vs STM32F1 (10..17 -> SAMPTR1, 0..9 ->
 *    SAMPTR2)
 *  - CTLR2.ALIGN: 0 = right-aligned, 1 = left-aligned
 *  - calibration additionally sets SAMPTR1 bits 29:27 to 0x7
 *
 * The driver is polled: ANIOC_TRIGGER converts the whole channel group
 * (software start, bounded EOC wait) and pushes the results through
 * adc_receive().  No DMA and no ADC interrupt are involved, so the
 * device works standalone; the DMA manager (ch32h41x_dma.c) can take
 * over the regular group later for higher-rate use.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>
#include <nuttx/kmalloc.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_adc.h"

#include "ch32h41x_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ADC1_BASE   CH32H41X_ADC1_BASE

/* Bounded register-wait loops.  The ADC runs from a 13MHz-class clock
 * during calibration; 100k polls is far beyond any documented latency
 * while still guaranteeing the driver cannot spin forever. */

#define ADC_SPINLIMIT 100000ul

/* The conversion group: internal channels only, so the device can be
 * verified without any external wiring.  Order defines the FIFO order.
 * (17 = Vrefint, 16 = temperature sensor). */

#define ADC_GROUP_NCHANNELS 2
static const uint8_t g_adc_group[ADC_GROUP_NCHANNELS] =
{
  CH32H41X_ADC_CHAN_VREFINT,
  CH32H41X_ADC_CHAN_TSENSE
};

/* USBHS PLL divider for the ADC clock: /36 while calibrating (EVT
 * RCC_USBHS_Div36), /6 in operation (EVT RCC_USBHS_Div6) */

#define ADC_CLK_CAL_DIV  36ul
#define ADC_CLK_RUN_DIV  6ul

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_adc_s
{
  FAR struct adc_dev_s *dev;      /* upper-half device */
  FAR const struct adc_callback_s *cb;  /* sample push (au_receive) */
  bool ready;                     /* set once the bring-up completed */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ch32h41x_adc_s g_adcdev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_adc_usbhspll
 *
 * Description:
 *   Bring up the USBHS PLL from the HSI reference (EVT USBHS_PLL()):
 *   select the source, poll the mux readback, enable the PLL and wait
 *   for lock.  Returns true when the PLL is ready.
 *
 ****************************************************************************/

static bool ch32h41x_adc_usbhspll(void)
{
  uint32_t timeout = ADC_SPINLIMIT;

  /* Reference divider + source = HSI (EVT PLLCFGR2 bit0 = 1) */

  modifyreg32(CH32H41X_RCC_PLLCFGR2,
              CH32H41X_RCC_USBHSPLL_REFSEL_MASK |
              CH32H41X_RCC_USBHSPLLSRC,
              CH32H41X_RCC_USBHSPLLSRC_HSI);

  /* Wait until the source mux readback confirms the HSI (sync poll,
   * EVT: while ((PLLCFGR2 & USBHSPLLSRC) != 0x01)) */

  while ((getreg32(CH32H41X_RCC_PLLCFGR2) &
          CH32H41X_RCC_USBHSPLLSRC) !=
         CH32H41X_RCC_USBHSPLLSRC_HSI)
    {
      if (--timeout == 0)
        {
          return false;
        }
    }

  /* PLL on + wait for lock */

  modifyreg32(CH32H41X_RCC_CTLR, 0, CH32H41X_RCC_USBHS_PLLON);

  timeout = ADC_SPINLIMIT;
  while ((getreg32(CH32H41X_RCC_CTLR) & CH32H41X_RCC_USBHS_PLLRDY) == 0)
    {
      if (--timeout == 0)
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: ch32h41x_adc_setclk
 *
 * Description:
 *   Select the USBHS PLL as the ADC clock source and set the divider.
 *
 ****************************************************************************/

static void ch32h41x_adc_setclk(uint32_t div)
{
  modifyreg32(CH32H41X_RCC_CFGR0,
              CH32H41X_RCC_ADCSRC | CH32H41X_RCC_ADCDIV_MASK,
              CH32H41X_RCC_ADCSRC | CH32H41X_RCC_ADCDIV(div));
}

/****************************************************************************
 * Name: ch32h41x_adc_calibrate
 *
 * Description:
 *   Run the vendor calibration sequence (EVT ADC_Function_Init):
 *   reset calibration, wait the flag to clear, arm SAMPTR1 bits 29:27
 *   (required on this chip), start calibration, wait for completion.
 *   Returns true on success.
 *
 ****************************************************************************/

static bool ch32h41x_adc_calibrate(uintptr_t base)
{
  uint32_t timeout = ADC_SPINLIMIT;

  /* Reset the calibration latches and wait for them to clear */

  modifyreg32(CH32H41X_ADC_CTLR2(base), 0, CH32H41X_ADC_CR2_RSTCAL);
  while ((getreg32(CH32H41X_ADC_CTLR2(base)) & CH32H41X_ADC_CR2_RSTCAL) != 0)
    {
      if (--timeout == 0)
        {
          return false;
        }
    }

  /* This chip arms calibration through SAMPTR1 bits 29:27 as well
   * (EVT ADC_StartCalibration: SAMPTR1 |= 0x7 << 27) */

  modifyreg32(CH32H41X_ADC_SAMPTR1(base), 0, 0x7ul << 27);

  /* Start calibration and wait for the self-clearing CAL bit */

  modifyreg32(CH32H41X_ADC_CTLR2(base), 0, CH32H41X_ADC_CR2_CAL);

  timeout = ADC_SPINLIMIT;
  while ((getreg32(CH32H41X_ADC_CTLR2(base)) & CH32H41X_ADC_CR2_CAL) != 0)
    {
      if (--timeout == 0)
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: ch32h41x_adc_one
 *
 * Description:
 *   Convert one channel in single-conversion mode: program the sample
 *   time and the regular sequence (rank 1 only), software-start, wait
 *   for EOC and return the right-aligned result.  Returns a negative
 *   errno on timeout.
 *
 ****************************************************************************/

static int ch32h41x_adc_one(uintptr_t base, uint8_t channel,
                            int32_t *result)
{
  uint32_t reg;
  uint32_t timeout = ADC_SPINLIMIT;

  /* Sample time field: channels 10..17 live in SAMPTR1 at 3*(ch-10),
   * channels 0..9 in SAMPTR2 at 3*ch (swapped vs STM32F1).  Use the
   * EVT ADC_SPEED_High sample mode 5. */

  if (channel > 9)
    {
      reg = getreg32(CH32H41X_ADC_SAMPTR1(base));
      reg &= ~(7ul << (3 * (channel - 10)));
      reg |= 5ul << (3 * (channel - 10));
      putreg32(reg, CH32H41X_ADC_SAMPTR1(base));
    }
  else
    {
      reg = getreg32(CH32H41X_ADC_SAMPTR2(base));
      reg &= ~(7ul << (3 * channel));
      reg |= 5ul << (3 * channel);
      putreg32(reg, CH32H41X_ADC_SAMPTR2(base));
    }

  /* Regular sequence: length 1 (RSQR1.L = 0), rank 1 = channel */

  reg = getreg32(CH32H41X_ADC_RSQR1(base));
  reg &= ~CH32H41X_ADC_RSQR1_L_MASK;
  putreg32(reg, CH32H41X_ADC_RSQR1(base));

  reg = getreg32(CH32H41X_ADC_RSQR3(base));
  reg &= ~CH32H41X_ADC_RSQR3_SQ1_MASK;
  reg |= channel & CH32H41X_ADC_RSQR3_SQ1_MASK;
  putreg32(reg, CH32H41X_ADC_RSQR3(base));

  /* Software start (EXTTRIG | SWSTART) and wait for EOC */

  modifyreg32(CH32H41X_ADC_CTLR2(base), 0,
              CH32H41X_ADC_CR2_EXTTRIG | CH32H41X_ADC_CR2_SWSTART);

  while ((getreg32(CH32H41X_ADC_STATR(base)) & CH32H41X_ADC_SR_EOC) == 0)
    {
      if (--timeout == 0)
        {
          return -ETIMEDOUT;
        }
    }

  /* Read clears EOC on this family; read the result anyway before
   * clearing defensively (write 0 to the flag bit) */

  *result = (int32_t)(getreg32(CH32H41X_ADC_RDATAR(base)) & 0xfffful);
  modifyreg32(CH32H41X_ADC_STATR(base), CH32H41X_ADC_SR_EOC, 0);
  return OK;
}

/****************************************************************************
 * Name: ch32h41x_adc_trigger
 *
 * Description:
 *   Convert the whole group and push the samples into the upper-half
 *   FIFO.  Returns the number of samples delivered, negative on error.
 *
 ****************************************************************************/

static int ch32h41x_adc_trigger(FAR struct ch32h41x_adc_s *priv)
{
  uintptr_t base = ADC1_BASE;
  unsigned n;
  int ret = 0;

  if (!priv->ready)
    {
      return -EAGAIN;
    }

  for (n = 0; n < ADC_GROUP_NCHANNELS; n++)
    {
      int32_t sample;
      int rc = ch32h41x_adc_one(base, g_adc_group[n], &sample);

      if (rc < 0)
        {
          return rc;
        }

      if (priv->cb != NULL && priv->cb->au_receive != NULL)
        {
          priv->cb->au_receive(priv->dev, g_adc_group[n], sample);
        }

      ret++;
    }

  return ret;
}

/****************************************************************************
 * Name: ch32h41x_adc_bringup
 *
 * Description:
 *   Full hardware bring-up in the EVT order.  Returns true on success.
 *
 ****************************************************************************/

static bool ch32h41x_adc_bringup(void)
{
  uintptr_t base = ADC1_BASE;

  /* Bus clock: ADC1 lives on the HB2 bus (EVT RCC_HB2Periph_ADC1) */

  modifyreg32(CH32H41X_RCC_HB2PCENR, 0, CH32H41X_RCC_HB2_ADC1);

  /* USBHS PLL first - it feeds the ADC clock */

  if (!ch32h41x_adc_usbhspll())
    {
      return false;
    }

  /* ADC clock from the USBHS PLL, slow divider while calibrating
   * (EVT: RCC_ADCCLKConfig(USBHSPLL) + RCC_ADCUSBHSPLLCLKAsSourceConfig
   * (RCC_USBHS_Div36)) */

  ch32h41x_adc_setclk(ADC_CLK_CAL_DIV);

  /* Peripheral reset through the HB2 reset register (EVT ADC_DeInit) */

  modifyreg32(CH32H41X_RCC_HB2PRSTR, 0, CH32H41X_RCC_HB2R_ADC1);
  modifyreg32(CH32H41X_RCC_HB2PRSTR, CH32H41X_RCC_HB2R_ADC1, 0);

  /* CTLR1: independent mode, no scan, no interrupts; CTLR2: ADON plus
   * the "no external trigger" source selection (EXTSEL = 0b111, EVT
   * ADC_ExternalTrigConv_None) so the software start below cannot be
   * preempted by a timer trigger line */

  putreg32(0, CH32H41X_ADC_CTLR1(base));
  putreg32(CH32H41X_ADC_CR2_ADON | (0x7ul << 17),
           CH32H41X_ADC_CTLR2(base));

  /* Vendor calibration at the slow ADC clock */

  if (!ch32h41x_adc_calibrate(base))
    {
      return false;
    }

  /* Operational ADC clock (EVT: RCC_USBHS_Div6) */

  ch32h41x_adc_setclk(ADC_CLK_RUN_DIV);

  /* Internal channels: the temperature sensor and Vrefint need the
   * output buffer on (EVT Internal_Temperature demo: BufferCmd ENABLE,
   * TempSensorVrefintCmd ENABLE) */

  modifyreg32(CH32H41X_ADC_CTLR1(base), 0, CH32H41X_ADC_CR1_BUFEN);
  modifyreg32(CH32H41X_ADC_CTLR2(base), 0, CH32H41X_ADC_CR2_TSVREFE);

  return true;
}

/****************************************************************************
 * Name: adc lower-half callbacks
 ****************************************************************************/

static int ch32h41x_adc_bind(FAR struct adc_dev_s *dev,
                             FAR const struct adc_callback_s *callback)
{
  g_adcdev.dev = dev;
  g_adcdev.cb = callback;
  return OK;
}

static void ch32h41x_adc_reset(FAR struct adc_dev_s *dev)
{
  /* Reset happens before setup: drop the ready flag so a trigger
   * before the bring-up reports -EAGAIN instead of touching hardware */

  g_adcdev.ready = false;
}

static int ch32h41x_adc_setup(FAR struct adc_dev_s *dev)
{
  g_adcdev.dev = dev;
  g_adcdev.ready = ch32h41x_adc_bringup();

  return g_adcdev.ready ? OK : -EIO;
}

static void ch32h41x_adc_shutdown(FAR struct adc_dev_s *dev)
{
  g_adcdev.ready = false;
}

static void ch32h41x_adc_rxint(FAR struct adc_dev_s *dev, bool enable)
{
  /* Polled driver: nothing to enable */
}

static int ch32h41x_adc_ioctl(FAR struct adc_dev_s *dev, int cmd,
                              unsigned long arg)
{
  int ret;

  switch (cmd)
    {
      case ANIOC_TRIGGER:
        {
          ret = ch32h41x_adc_trigger(&g_adcdev);
        }
        break;

      default:
        {
          ret = -ENOTTY;
        }
        break;
    }

  return ret;
}

static const struct adc_ops_s g_adc_ops =
{
  .ao_bind     = ch32h41x_adc_bind,
  .ao_reset    = ch32h41x_adc_reset,
  .ao_setup    = ch32h41x_adc_setup,
  .ao_shutdown = ch32h41x_adc_shutdown,
  .ao_rxint    = ch32h41x_adc_rxint,
  .ao_ioctl    = ch32h41x_adc_ioctl,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_adc_initialize
 *
 * Description:
 *   Register ADC1 as /dev/adc0.  The hardware is brought up on the
 *   first open (ao_setup).
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int ch32h41x_adc_initialize(void)
{
  static FAR struct adc_dev_s *g_adcreg;
  FAR struct adc_dev_s *dev;

  if (g_adcreg != NULL)
    {
      return OK;
    }

  /* Allocate the full device structure: the 12.12 adc_register() binds
   * a caller-built adc_dev_s (ad_ops + ad_priv), it does not build one */

  dev = kmm_zalloc(sizeof(struct adc_dev_s));
  if (dev == NULL)
    {
      return -ENOMEM;
    }

  dev->ad_ops = &g_adc_ops;

  if (adc_register("/dev/adc0", dev) != OK)
    {
      kmm_free(dev);
      return -EINVAL;
    }

  g_adcreg = dev;
  return OK;
}
