/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_dma.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>

#include <nuttx/irq.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_dma.h"

#include "ch32h41x_dma.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* DMA controller register blocks (EVT DMA_TypeDef: INTFR @ +0x00,
 * INTFCR @ +0x04; channels at +0x08, stride 0x14). */

#define CH32H41X_DMA_BLOCK(c) \
  ((c) == 0 ? CH32H41X_DMA1_BASE : CH32H41X_DMA2_BASE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Allocation state of the 16 DMA channels (DMA1_CH1..8, DMA2_CH1..8) */

static uint8_t g_dma_used[2][8];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_dma_clock
 *
 * Description:
 *   Enable the bus clock for the given DMA controller.  DMA1/DMA2 live
 *   on the HB bus (EVT RCC_HBPeriph_DMA1/DMA2, HBPCENR bits 0/1).
 *
 ****************************************************************************/

static void ch32h41x_dma_clock(uint8_t controller)
{
  uint32_t bit = (controller == 0) ? CH32H41X_RCC_HB_DMA1 :
                                     CH32H41X_RCC_HB_DMA2;

  modifyreg32(CH32H41X_RCC_HBPCENR, 0, bit);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_dma_alloc
 *
 * Description:
 *   Reserve a DMA channel.  The caller names the controller (0 = DMA1,
 *   1 = DMA2) and the on-controller channel (0..7, i.e. DMA1_CH1 is
 *   controller 0 channel 0).
 *
 * Returned Value:
 *   Zero on success; -EBUSY when the channel is already in use.
 *
 ****************************************************************************/

int ch32h41x_dma_alloc(uint8_t controller, uint8_t channel)
{
  irqstate_t flags;
  int ret = OK;

  DEBUGASSERT(controller < 2 && channel < 8);

  flags = enter_critical_section();

  if (g_dma_used[controller][channel] != 0)
    {
      ret = -EBUSY;
    }
  else
    {
      g_dma_used[controller][channel] = 1;
      ch32h41x_dma_clock(controller);
    }

  leave_critical_section(flags);
  return ret;
}

/****************************************************************************
 * Name: ch32h41x_dma_free
 *
 * Description:
 *   Release a previously allocated channel.  The channel is disabled and
 *   its interrupt flags cleared so the next user starts clean.
 *
 ****************************************************************************/

void ch32h41x_dma_free(uint8_t controller, uint8_t channel)
{
  uintptr_t ch = CH32H41X_DMA_CH_BASE(CH32H41X_DMA_BLOCK(controller),
                                      channel);

  DEBUGASSERT(controller < 2 && channel < 8);

  /* Disable the channel (the EN bit self-clears on transfer complete in
   * normal mode; a forced clear makes free safe in all states). */

  modifyreg32(CH32H41X_DMA_CH_CFGR(ch), CH32H41X_DMA_CFGR_EN, 0);

  /* Wipe the previous transfer's state: reset the counters/addresses and
   * clear the sticky interrupt flags in the controller status register. */

  putreg32(0, CH32H41X_DMA_CH_CNTR(ch));
  putreg32(0, CH32H41X_DMA_CH_PADDR(ch));
  putreg32(0, CH32H41X_DMA_CH_MADDR(ch));
  putreg32(CH32H41X_DMA_CFLAGS(channel),
           CH32H41X_DMA_INTFCR(CH32H41X_DMA_BLOCK(controller)));

  g_dma_used[controller][channel] = 0;
}

/****************************************************************************
 * Name: ch32h41x_dma_config
 *
 * Description:
 *   Program a channel for one transfer: peripheral and memory base
 *   addresses, transfer count and the CFGR control bits.  The caller
 *   builds 'cfgr' from the CH32H41X_DMA_CFGR_* definitions (direction,
 *   increments, sizes, priority, circular, double-buffer).  Interrupt
 *   enables are part of cfgr; without a dispatched ISR they must be
 *   left off and the flags polled instead.
 *
 ****************************************************************************/

void ch32h41x_dma_config(uint8_t controller, uint8_t channel,
                         uint32_t paddr, uint32_t maddr,
                         uint16_t count, uint32_t cfgr)
{
  uintptr_t ch = CH32H41X_DMA_CH_BASE(CH32H41X_DMA_BLOCK(controller),
                                      channel);

  DEBUGASSERT(controller < 2 && channel < 8);

  /* The channel must be off while its registers are reprogrammed */

  modifyreg32(CH32H41X_DMA_CH_CFGR(ch), CH32H41X_DMA_CFGR_EN, 0);

  putreg32(cfgr & ~CH32H41X_DMA_CFGR_EN, CH32H41X_DMA_CH_CFGR(ch));
  putreg32(count, CH32H41X_DMA_CH_CNTR(ch));
  putreg32(paddr, CH32H41X_DMA_CH_PADDR(ch));
  putreg32(maddr, CH32H41X_DMA_CH_MADDR(ch));
}

/****************************************************************************
 * Name: ch32h41x_dma_muxreq
 *
 * Description:
 *   Route a peripheral request into a DMA channel through the DMAMUX.
 *   'request' is the peripheral request number (e.g.
 *   CH32H41X_DMAMUX_REQ_ADC1); the mux register field receives
 *   request-1 (EVT DMA_MuxChannelConfig).  Only change the mux while
 *   the channel is disabled.
 *
 ****************************************************************************/

void ch32h41x_dma_muxreq(uint8_t controller, uint8_t channel,
                         uint32_t request)
{
  uint32_t mux = controller * 8ul + channel;
  uintptr_t reg = CH32H41X_DMAMUX_CFGR(mux);
  uint32_t shift = CH32H41X_DMAMUX_SHIFT(mux);

  DEBUGASSERT(controller < 2 && channel < 8);
  DEBUGASSERT(request >= 1 && request <= 0x7bul);

  modifyreg32(reg, CH32H41X_DMAMUX_FIELD(mux),
              (request - 1ul) << shift);
}

/****************************************************************************
 * Name: ch32h41x_dma_enable / ch32h41x_dma_count /
 *       ch32h41x_dma_flags / ch32h41x_dma_clearflags
 *
 * Description:
 *   Start/stop a configured channel and inspect its progress.  The
 *   flag helpers expose the 4-bit per-channel status group (GIF/TCIF/
 *   HTIF/TEIF) for polled transfers.
 *
 ****************************************************************************/

void ch32h41x_dma_enable(uint8_t controller, uint8_t channel, bool enable)
{
  uintptr_t ch = CH32H41X_DMA_CH_BASE(CH32H41X_DMA_BLOCK(controller),
                                      channel);

  DEBUGASSERT(controller < 2 && channel < 8);

  if (enable)
    {
      modifyreg32(CH32H41X_DMA_CH_CFGR(ch), 0, CH32H41X_DMA_CFGR_EN);
    }
  else
    {
      modifyreg32(CH32H41X_DMA_CH_CFGR(ch), CH32H41X_DMA_CFGR_EN, 0);
    }
}

uint16_t ch32h41x_dma_count(uint8_t controller, uint8_t channel)
{
  uintptr_t ch = CH32H41X_DMA_CH_BASE(CH32H41X_DMA_BLOCK(controller),
                                      channel);

  DEBUGASSERT(controller < 2 && channel < 8);

  return (uint16_t)(getreg32(CH32H41X_DMA_CH_CNTR(ch)) & 0xfffful);
}

uint32_t ch32h41x_dma_flags(uint8_t controller, uint8_t channel)
{
  uint32_t status = getreg32(CH32H41X_DMA_INTFR(CH32H41X_DMA_BLOCK(controller)));
  uint32_t shift = 4ul * channel;

  DEBUGASSERT(controller < 2 && channel < 8);

  return (status >> shift) & 0x7ul;
}

void ch32h41x_dma_clearflags(uint8_t controller, uint8_t channel)
{
  DEBUGASSERT(controller < 2 && channel < 8);

  putreg32(CH32H41X_DMA_CFLAGS(channel),
           CH32H41X_DMA_INTFCR(CH32H41X_DMA_BLOCK(controller)));
}
