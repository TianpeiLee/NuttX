/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_usart.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_USART_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_USART_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* USART register offsets (16-bit registers, 32-bit stride) */

#define CH32H41X_USART_STATR    0x00  /* status */
#define CH32H41X_USART_DATAR    0x04  /* data */
#define CH32H41X_USART_BRR      0x08  /* baud rate */
#define CH32H41X_USART_CTLR1    0x0c  /* control 1 */
#define CH32H41X_USART_CTLR2    0x10  /* control 2 */
#define CH32H41X_USART_CTLR3    0x14  /* control 3 */
#define CH32H41X_USART_GPR      0x18  /* guard time and prescaler */
#define CH32H41X_USART_CTLR4    0x1c  /* control 4 */

/* STATR bits */

#define CH32H41X_USART_SR_PEF   (1ul << 0)    /* parity error flag */
#define CH32H41X_USART_SR_FEF   (1ul << 1)    /* framing error flag */
#define CH32H41X_USART_SR_NEF   (1ul << 2)    /* noise error flag */
#define CH32H41X_USART_SR_OREF  (1ul << 3)    /* overrun error flag */
#define CH32H41X_USART_SR_IDLEF (1ul << 4)    /* idle line detected */
#define CH32H41X_USART_SR_RXNEF (1ul << 5)    /* read data register not empty */
#define CH32H41X_USART_SR_TCF   (1ul << 6)    /* transmission complete */
#define CH32H41X_USART_SR_TXEF  (1ul << 7)    /* transmit data register empty */
#define CH32H41X_USART_SR_ERROR_MASK (CH32H41X_USART_SR_OREF | \
                                      CH32H41X_USART_SR_NEF | \
                                      CH32H41X_USART_SR_FEF | \
                                      CH32H41X_USART_SR_PEF)

/* CTLR1 bits */

#define CH32H41X_USART_CR1_UE   (1ul << 13)   /* USART enable */
#define CH32H41X_USART_CR1_M    (1ul << 12)   /* word length: 0=8bit, 1=9bit */
#define CH32H41X_USART_CR1_WAKE (1ul << 11)   /* wakeup method */
#define CH32H41X_USART_CR1_PCE  (1ul << 10)   /* parity control enable */
#define CH32H41X_USART_CR1_PS   (1ul << 9)    /* parity selection: 0=even */
#define CH32H41X_USART_CR1_PEIE (1ul << 8)    /* parity error interrupt enable */
#define CH32H41X_USART_CR1_TXEIE (1ul << 7)   /* TXE interrupt enable */
#define CH32H41X_USART_CR1_TCIE (1ul << 6)    /* transmission complete int enable */
#define CH32H41X_USART_CR1_RXNEIE (1ul << 5)  /* RXNE interrupt enable */
#define CH32H41X_USART_CR1_IDLEIE (1ul << 4)  /* idle interrupt enable */
#define CH32H41X_USART_CR1_TE   (1ul << 3)    /* transmitter enable */
#define CH32H41X_USART_CR1_RE   (1ul << 2)    /* receiver enable */
#define CH32H41X_USART_CR1_RWU  (1ul << 1)    /* receiver wakeup */
#define CH32H41X_USART_CR1_SMBK (1ul << 0)    /* break character send */

/* CTLR2 bits */

#define CH32H41X_USART_CR2_STOP_SHIFT 12      /* STOP bits */
#define CH32H41X_USART_CR2_STOP_1 (0x0ul << CH32H41X_USART_CR2_STOP_SHIFT)
#define CH32H41X_USART_CR2_STOP_2 (0x2ul << CH32H41X_USART_CR2_STOP_SHIFT)
#define CH32H41X_USART_CR2_STOP_15 (0x1ul << CH32H41X_USART_CR2_STOP_SHIFT)

/* CTLR3 bits */

#define CH32H41X_USART_CR3_CTSE (1ul << 9)    /* CTS enable */
#define CH32H41X_USART_CR3_RTSE (1ul << 8)    /* RTS enable */
#define CH32H41X_USART_CR3_DMAR (1ul << 6)    /* DMA enable receiver */
#define CH32H41X_USART_CR3_DMAT (1ul << 7)    /* DMA enable transmitter */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_USART_H */
