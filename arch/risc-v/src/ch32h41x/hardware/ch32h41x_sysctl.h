/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_sysctl.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_SYSCTL_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_SYSCTL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* PFIC (Programmable Fast Interrupt Controller) register offsets.
 *
 * The PFIC covers vector indices 0..148.  Array registers below are
 * indexed by (vector index / 32).
 */

#define CH32H41X_PFIC_ISR(n)     (CH32H41X_PFIC_BASE + (n) * 4)          /* 0x00: interrupt status */
#define CH32H41X_PFIC_IPR(n)     (CH32H41X_PFIC_BASE + 0x20 + (n) * 4)   /* 0x20: interrupt pending */
#define CH32H41X_PFIC_ITHRESDR   (CH32H41X_PFIC_BASE + 0x40)             /* 0x40: interrupt threshold */
#define CH32H41X_PFIC_CFGR       (CH32H41X_PFIC_BASE + 0x48)             /* 0x48: configuration */
#define CH32H41X_PFIC_GISR       (CH32H41X_PFIC_BASE + 0x4c)             /* 0x4c: interrupt group status */
#define CH32H41X_PFIC_VTFIDR(n)  (CH32H41X_PFIC_BASE + 0x50 + (n))       /* 0x50: VTF IRQ select */
#define CH32H41X_PFIC_VTFADDR(n) (CH32H41X_PFIC_BASE + 0x60 + (n) * 4)   /* 0x60: VTF address */
#define CH32H41X_PFIC_IENR(n)    (CH32H41X_PFIC_BASE + 0x100 + (n) * 4)  /* 0x100: interrupt enable set */
#define CH32H41X_PFIC_IRER(n)    (CH32H41X_PFIC_BASE + 0x180 + (n) * 4)  /* 0x180: interrupt enable reset */
#define CH32H41X_PFIC_IPSR(n)    (CH32H41X_PFIC_BASE + 0x200 + (n) * 4)  /* 0x200: interrupt pending set */
#define CH32H41X_PFIC_IPRR(n)    (CH32H41X_PFIC_BASE + 0x280 + (n) * 4)  /* 0x280: interrupt pending reset */
#define CH32H41X_PFIC_IACTR(n)   (CH32H41X_PFIC_BASE + 0x300 + (n) * 4)  /* 0x300: interrupt active */
#define CH32H41X_PFIC_IPRIOR(n)  (CH32H41X_PFIC_BASE + 0x400 + (n))      /* 0x400: interrupt priority (byte/IRQ) */
#define CH32H41X_PFIC_IALLOCR(n) (CH32H41X_PFIC_BASE + 0x500 + (n))      /* 0x500: dual-core IRQ allocation */
#define CH32H41X_PFIC_IAUTR(n)   (CH32H41X_PFIC_BASE + 0x600 + (n) * 4)  /* 0x600: auto allocation */
#define CH32H41X_PFIC_WAKEIP(n)  (CH32H41X_PFIC_BASE + 0x720 + (n) * 4)  /* 0x620: core wake address */
#define CH32H41X_PFIC_CSTAR(n)   (CH32H41X_PFIC_BASE + 0x78c + (n) * 4)  /* 0x680: core start */
#define CH32H41X_PFIC_SCTLR      (CH32H41X_PFIC_BASE + 0xd10)            /* 0xef0: system control */

#define CH32H41X_PFIC_IRQWORD(irq)  ((irq) >> 5)
#define CH32H41X_PFIC_IRQBIT(irq)   (1ul << ((irq) & 0x1f))

/* PFIC_CFGR bits */

#define CH32H41X_PFIC_CFGR_KEY      0xbeef0000ul  /* unlock key */
#define CH32H41X_PFIC_CFGR_SYSRST   (1ul << 7)    /* system reset request */
#define CH32H41X_PFIC_CFGR_STKMOD   (1ul << 15)   /* SysTick direction */

/* Core ID in PFIC_SCTLR */

#define CH32H41X_PFIC_SCTLR_COREID  (1ul << 16)   /* 0=V3F, 1=V5F */

/* SysTick (CH32H41X_SYSTICK0_BASE / CH32H41X_SYSTICK1_BASE) register offsets */

#define CH32H41X_STK_CTLR           0x00  /* control */
#define CH32H41X_STK_ISR            0x04  /* status (only SysTick0) */
#define CH32H41X_STK_CNT            0x08  /* counter */
#define CH32H41X_STK_CMP            0x10  /* compare */

/* SysTick CTLR bits */

#define CH32H41X_STK_CTLR_STE       (1ul << 0)    /* system timer enable */
#define CH32H41X_STK_CTLR_STIE      (1ul << 1)    /* interrupt enable */
#define CH32H41X_STK_CTLR_STCLK     (1ul << 2)    /* 1: HCLK, 0: HCLK/8 */
#define CH32H41X_STK_CTLR_STRE      (1ul << 3)    /* auto reload on compare */
#define CH32H41X_STK_CTLR_MODE      (1ul << 4)    /* 1: down-count, 0: up-count */
#define CH32H41X_STK_CTLR_SWIE      (1ul << 31)   /* software trigger */

/* SysTick ISR bits */

#define CH32H41X_STK_ISR_UIF        (1ul << 0)    /* compare interrupt flag (write 0 to clear) */

/* Core CSR numbers (custom, WCH QingKe) */

#define CH32H41X_CSR_INTCFG         0x804   /* interrupt prologue / nesting config */
#define CH32H41X_CSR_CACHECFG       0xbc0   /* cache/prefetch configuration */
#define CH32H41X_CSR_NESTCFG        0xbc1   /* hardware nesting level */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_SYSCTL_H */
