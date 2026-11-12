/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_dma.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_DMA_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_DMA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Controller register addresses (DMA1/DMA2) */

#define CH32H41X_DMA_INTFR(d)    ((d) + 0x00)   /* interrupt status */
#define CH32H41X_DMA_INTFCR(d)   ((d) + 0x04)   /* interrupt flag clear */

/* Channel register block: DMA1_CH1 at 0x40020008, stride 0x14, 8 channels
 * per controller. */

#define CH32H41X_DMA_CH_BASE(d, n)   ((d) + 0x08 + (n) * 0x14)
#define CH32H41X_DMA_CH_CFGR(c)      ((c) + 0x00)   /* channel configuration */
#define CH32H41X_DMA_CH_CNTR(c)      ((c) + 0x04)   /* transfer count */
#define CH32H41X_DMA_CH_PADDR(c)     ((c) + 0x08)   /* peripheral address */
#define CH32H41X_DMA_CH_MADDR(c)     ((c) + 0x0c)   /* memory address */
#define CH32H41X_DMA_CH_M1ADDR(c)    ((c) + 0x10)   /* memory 1 address (double buffer) */

#define CH32H41X_DMA1_CHAN(n)    CH32H41X_DMA_CH_BASE(CH32H41X_DMA1_BASE, (n))
#define CH32H41X_DMA2_CHAN(n)    CH32H41X_DMA_CH_BASE(CH32H41X_DMA2_BASE, (n))

/* CFGR bits (values from EVT ch32h417_dma.h API constants) */

#define CH32H41X_DMA_CFGR_EN       (1ul << 0)   /* channel enable */
#define CH32H41X_DMA_CFGR_TCIE     (1ul << 1)   /* transfer complete int */
#define CH32H41X_DMA_CFGR_HTIE     (1ul << 2)   /* half transfer int */
#define CH32H41X_DMA_CFGR_TEIE     (1ul << 3)   /* transfer error int */
#define CH32H41X_DMA_CFGR_DIR      (1ul << 4)   /* 0=periph->mem, 1=mem->periph */
#define CH32H41X_DMA_CFGR_CIRC     (1ul << 5)   /* circular mode */
#define CH32H41X_DMA_CFGR_PINC     (1ul << 6)   /* peripheral increment */
#define CH32H41X_DMA_CFGR_MINC     (1ul << 7)   /* memory increment */
#define CH32H41X_DMA_CFGR_PSIZE_SHIFT 8         /* peripheral size (0=byte,1=half,2=word,3=256bit) */
#define CH32H41X_DMA_CFGR_PSIZE_MASK (3ul << CH32H41X_DMA_CFGR_PSIZE_SHIFT)
#define CH32H41X_DMA_CFGR_MSIZE_SHIFT 10        /* memory size */
#define CH32H41X_DMA_CFGR_MSIZE_MASK (3ul << CH32H41X_DMA_CFGR_MSIZE_SHIFT)
#define CH32H41X_DMA_CFGR_PL_SHIFT 12           /* priority */
#define CH32H41X_DMA_CFGR_PL_MASK  (3ul << CH32H41X_DMA_CFGR_PL_SHIFT)
#define CH32H41X_DMA_CFGR_M2M      (1ul << 14)  /* memory-to-memory */

/* INTFR/INTFCR: 4 bits per channel n at bit offset 4*n: GIF, TCIF, HTIF,
 * TEIF (write-1-to-clear via INTFCR). */

#define CH32H41X_DMA_GIF(n)      (1ul << (4 * (n)))
#define CH32H41X_DMA_TCIF(n)     (1ul << (4 * (n) + 1))
#define CH32H41X_DMA_HTIF(n)     (1ul << (4 * (n) + 2))
#define CH32H41X_DMA_TEIF(n)     (1ul << (4 * (n) + 3))
#define CH32H41X_DMA_CFLAGS(n)   (0xful << (4 * (n)))

/* DMAMUX: four 32-bit registers at CH32H41X_DMAMUX_BASE, each packing
 * FOUR 8-bit channel fields (EVT DMAMUX_TypeDef: CFGR0_3/CFGR4_7/
 * CFGR8_11/CFGR12_15).  Global mux channel m (DMA1_CH1..8 = 0..7,
 * DMA2_CH1..8 = 8..15) lives in register m/4, byte m%4.  Each field is
 * a 7-bit request number written as (request - 1); requests range
 * 1..0x7B (EVT DMA_MuxChannelConfig).  NOTE: this is NOT the STM32-style
 * per-channel 32-bit CCR with an enable bit - there is no enable bit. */

#define CH32H41X_DMAMUX_CFGR(m)  (CH32H41X_DMAMUX_BASE + ((m) / 4ul) * 4ul)
#define CH32H41X_DMAMUX_SHIFT(m) (((m) % 4ul) * 8ul)
#define CH32H41X_DMAMUX_FIELD(m) (0x7ful << CH32H41X_DMAMUX_SHIFT(m))

/* Request numbers (verified against the EVT demos; ADC1 from
 * EXAM/ADC/ADC_DMA/Common/hardware.c).  Extend this list only from
 * EVT demo code or the reference manual - do not guess. */

#define CH32H41X_DMAMUX_REQ_ADC1 0x78ul

/* Double-buffer mode bit (EVT DMA_BufferMode_Memory) */

#define CH32H41X_DMA_CFGR_DBM      (1ul << 15)   /* double buffer mode */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_DMA_H */
