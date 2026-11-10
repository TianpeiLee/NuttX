/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_spi.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_SPI_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_SPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses (16-bit registers on 32-bit stride) */

#define CH32H41X_SPI_CTLR1(s)    ((s) + 0x00)
#define CH32H41X_SPI_CTLR2(s)    ((s) + 0x04)
#define CH32H41X_SPI_STATR(s)    ((s) + 0x08)
#define CH32H41X_SPI_DATAR(s)    ((s) + 0x0c)
#define CH32H41X_SPI_CRCR(s)     ((s) + 0x10)
#define CH32H41X_SPI_RCRCR(s)    ((s) + 0x14)
#define CH32H41X_SPI_TCRCR(s)    ((s) + 0x18)
#define CH32H41X_SPI_I2SCFGR(s)  ((s) + 0x1c)
#define CH32H41X_SPI_I2SPR(s)    ((s) + 0x20)
#define CH32H41X_SPI_HSCR(s)     ((s) + 0x24)

/* CTLR1 bits (values cross-checked against the EVT ch32h417_spi.h
 * API constants: SPI_Mode_Master=0x104, SPI_NSS_Soft=0x200,
 * SPI_BaudRatePrescaler_Mode7=0x38, SPI_FirstBit_LSB=0x80,
 * SPI_DataSize_16b=0x800, SPI_CPOL_High=0x2, SPI_CPHA_2Edge=0x1) */

#define CH32H41X_SPI_CR1_CPHA     (1ul << 0)     /* clock phase */
#define CH32H41X_SPI_CR1_CPOL     (1ul << 1)     /* clock polarity */
#define CH32H41X_SPI_CR1_MSTR     (1ul << 2)     /* master selection */
#define CH32H41X_SPI_CR1_BR_SHIFT 3               /* baud rate prescaler */
#define CH32H41X_SPI_CR1_BR_MASK  (7ul << CH32H41X_SPI_CR1_BR_SHIFT)
#define CH32H41X_SPI_CR1_BR(n)    (((n) & 7) << CH32H41X_SPI_CR1_BR_SHIFT)
#define CH32H41X_SPI_CR1_SPE      (1ul << 6)     /* SPI enable */
#define CH32H41X_SPI_CR1_LSBFIRST (1ul << 7)     /* frame format */
#define CH32H41X_SPI_CR1_SSI      (1ul << 8)     /* internal slave select */
#define CH32H41X_SPI_CR1_SSM      (1ul << 9)     /* software slave management */
#define CH32H41X_SPI_CR1_RXONLY   (1ul << 10)    /* receive only */
#define CH32H41X_SPI_CR1_DFF      (1ul << 11)    /* data frame format (1=16bit) */
#define CH32H41X_SPI_CR1_CRCNEXT  (1ul << 12)    /* transmit CRC next */
#define CH32H41X_SPI_CR1_CRCEN    (1ul << 13)    /* hardware CRC enable */
#define CH32H41X_SPI_CR1_BIDIOE   (1ul << 14)    /* bidirectional output enable */
#define CH32H41X_SPI_CR1_BIDIMODE (1ul << 15)    /* bidirectional data mode */

/* CTLR2 bits */

#define CH32H41X_SPI_CR2_RXDMAEN  (1ul << 0)     /* RX buffer DMA enable */
#define CH32H41X_SPI_CR2_TXDMAEN  (1ul << 1)     /* TX buffer DMA enable */
#define CH32H41X_SPI_CR2_ERRIE    (1ul << 5)     /* error interrupt enable */
#define CH32H41X_SPI_CR2_RXNEIE   (1ul << 6)     /* RX not empty interrupt */
#define CH32H41X_SPI_CR2_TXEIE    (1ul << 7)     /* TX empty interrupt */
#define CH32H41X_SPI_CR2_SSOE     (1ul << 9)     /* SS output enable */

/* STATR bits */

#define CH32H41X_SPI_SR_RXNE      (1ul << 0)     /* receive buffer not empty */
#define CH32H41X_SPI_SR_TXE       (1ul << 1)     /* transmit buffer empty */
#define CH32H41X_SPI_SR_CHSIDE    (1ul << 2)     /* channel side */
#define CH32H41X_SPI_SR_UDR       (1ul << 3)     /* underrun flag */
#define CH32H41X_SPI_SR_CRCERR    (1ul << 4)     /* CRC error (write 0 clear) */
#define CH32H41X_SPI_SR_MODF      (1ul << 5)     /* mode fault (write 0 clear) */
#define CH32H41X_SPI_SR_OVR       (1ul << 6)     /* overrun (write 0 clear) */
#define CH32H41X_SPI_SR_BSY       (1ul << 7)     /* busy flag */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_SPI_H */
