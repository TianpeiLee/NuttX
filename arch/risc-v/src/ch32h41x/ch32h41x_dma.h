/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_dma.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_CH32H41X_DMA_H
#define __ARCH_RISCV_SRC_CH32H41X_CH32H41X_DMA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/* Channel reservation.  Controller 0 = DMA1, 1 = DMA2; channel 0..7 is
 * the on-controller channel number (DMA1_CH1 == controller 0 channel 0). */

int ch32h41x_dma_alloc(uint8_t controller, uint8_t channel);
void ch32h41x_dma_free(uint8_t controller, uint8_t channel);

/* Program one transfer: peripheral/memory base addresses, transfer
 * count and the CFGR bits (CH32H41X_DMA_CFGR_*).  The channel must be
 * allocated and disabled; DMA interrupts are part of cfgr - leave them
 * off unless the IRQ dispatch is wired up, and poll the flags instead. */

void ch32h41x_dma_config(uint8_t controller, uint8_t channel,
                         uint32_t paddr, uint32_t maddr,
                         uint16_t count, uint32_t cfgr);

/* Route the peripheral request number (CH32H41X_DMAMUX_REQ_*) into the
 * channel through the DMAMUX.  Only while the channel is disabled. */

void ch32h41x_dma_muxreq(uint8_t controller, uint8_t channel,
                         uint32_t request);

/* Run control and progress reporting for polled transfers. */

void ch32h41x_dma_enable(uint8_t controller, uint8_t channel, bool enable);
uint16_t ch32h41x_dma_count(uint8_t controller, uint8_t channel);
uint32_t ch32h41x_dma_flags(uint8_t controller, uint8_t channel);
void ch32h41x_dma_clearflags(uint8_t controller, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* __ARCH_RISCV_SRC_CH32H41X_CH32H41X_DMA_H */
