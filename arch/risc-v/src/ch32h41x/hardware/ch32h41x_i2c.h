/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_i2c.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_I2C_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_I2C_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses (16-bit registers on 32-bit stride) */

#define CH32H41X_I2C_CTLR1(i)    ((i) + 0x00)
#define CH32H41X_I2C_CTLR2(i)    ((i) + 0x04)
#define CH32H41X_I2C_OADDR1(i)   ((i) + 0x08)
#define CH32H41X_I2C_OADDR2(i)   ((i) + 0x0c)
#define CH32H41X_I2C_DATAR(i)    ((i) + 0x10)
#define CH32H41X_I2C_STAR1(i)    ((i) + 0x14)
#define CH32H41X_I2C_STAR2(i)    ((i) + 0x18)
#define CH32H41X_I2C_CKCFGR(i)   ((i) + 0x1c)
#define CH32H41X_I2C_RTR(i)      ((i) + 0x20)

/* CTLR1 bits */

#define CH32H41X_I2C_CR1_PE      (1ul << 0)     /* peripheral enable */
#define CH32H41X_I2C_CR1_SMBUS   (1ul << 1)     /* SMBus mode */
#define CH32H41X_I2C_CR1_START   (1ul << 8)     /* start generation */
#define CH32H41X_I2C_CR1_STOP    (1ul << 9)     /* stop generation */
#define CH32H41X_I2C_CR1_ACK     (1ul << 10)    /* acknowledge enable */
#define CH32H41X_I2C_CR1_POS     (1ul << 11)    /* acknowledge/PEC position */
#define CH32H41X_I2C_CR1_PEC     (1ul << 12)    /* packet error checking */
#define CH32H41X_I2C_CR1_ENGC    (1ul << 13)    /* general call enable */
#define CH32H41X_I2C_CR1_ENPEC   (1ul << 14)    /* PEC enable */
#define CH32H41X_I2C_CR1_ENARP   (1ul << 15)    /* ARP enable */
#define CH32H41X_I2C_CR1_NOSTRETCH (1ul << 17)  /* clock stretching disable (slave) */
#define CH32H41X_I2C_CR1_SWRST   (1ul << 15)    /* NOTE: shares ENARP encoding in EVT; software reset via FCTLR-style write if needed */

/* CTLR2 bits */

#define CH32H41X_I2C_CR2_FREQ_SHIFT 0           /* peripheral clock freq (MHz) */
#define CH32H41X_I2C_CR2_FREQ_MASK (0x3ful << CH32H41X_I2C_CR2_FREQ_SHIFT)
#define CH32H41X_I2C_CR2_ITERREN (1ul << 8)     /* error interrupt enable */
#define CH32H41X_I2C_CR2_ITEVTEN (1ul << 9)     /* event interrupt enable */
#define CH32H41X_I2C_CR2_ITBUFEN (1ul << 10)    /* buffer interrupt enable */
#define CH32H41X_I2C_CR2_DMAEN   (1ul << 11)    /* DMA requests enable */
#define CH32H41X_I2C_CR2_LAST    (1ul << 12)    /* DMA last transfer */

/* OADDR1 bits */

#define CH32H41X_I2C_OA1_ADD0    (1ul << 0)     /* bit 0 (10-bit / general call) */
#define CH32H41X_I2C_OA1_ADD1_SHIFT 1           /* 7-bit address at ADD[7:1] */
#define CH32H41X_I2C_OA1_ADD1_MASK (0x7ful << CH32H41X_I2C_OA1_ADD1_SHIFT)
#define CH32H41X_I2C_OA1_ADDCONF (1ul << 15)    /* address configure mode */
#define CH32H41X_I2C_OA1_ADDCONF_7BIT  (0x0ul << 15)
#define CH32H41X_I2C_OA1_ADDCONF_10BIT (0x1ul << 15)

/* OADDR2 bits */

#define CH32H41X_I2C_OA2_ENDUAL  (1ul << 0)     /* dual addressing enable */
#define CH32H41X_I2C_OA2_ADD2_SHIFT 1
#define CH32H41X_I2C_OA2_ADD2_MASK (0x7ful << CH32H41X_I2C_OA2_ADD2_SHIFT)

/* STAR1 bits */

#define CH32H41X_I2C_SR1_SB      (1ul << 0)     /* start bit sent */
#define CH32H41X_I2C_SR1_ADDR    (1ul << 1)     /* address sent/matched */
#define CH32H41X_I2C_SR1_BTF     (1ul << 2)     /* byte transfer finished */
#define CH32H41X_I2C_SR1_ADD10   (1ul << 3)     /* 10-bit header sent */
#define CH32H41X_I2C_SR1_STOPF   (1ul << 4)     /* stop detection */
#define CH32H41X_I2C_SR1_RXNE    (1ul << 6)     /* data register not empty */
#define CH32H41X_I2C_SR1_TXE     (1ul << 7)     /* data register empty */
#define CH32H41X_I2C_SR1_BERR    (1ul << 8)     /* bus error */
#define CH32H41X_I2C_SR1_ARLO    (1ul << 9)     /* arbitration lost */
#define CH32H41X_I2C_SR1_AF      (1ul << 10)    /* acknowledge failure */
#define CH32H41X_I2C_SR1_OVR     (1ul << 11)    /* overrun/underrun */
#define CH32H41X_I2C_SR1_PECERR  (1ul << 12)    /* PEC error */
#define CH32H41X_I2C_SR1_TIMEOUT (1ul << 14)    /* timeout error */
#define CH32H41X_I2C_SR1_SMBALERT (1ul << 15)   /* SMBus alert */

/* STAR2 bits */

#define CH32H41X_I2C_SR2_MSL     (1ul << 0)     /* master/slave (in STAR2 low byte) */
#define CH32H41X_I2C_SR2_BUSY    (1ul << 1)
#define CH32H41X_I2C_SR2_TRA     (1ul << 2)
#define CH32H41X_I2C_SR2_GENCALL (1ul << 4)
#define CH32H41X_I2C_SR2_DUALF   (1ul << 6)
/* NOTE: STAR2 is read with the flags in bits 15:8 of the register on
 * WCH/STM32 F1 parts; the nibble shift is applied by the driver. */

#define CH32H41X_I2C_SR2_MSL_S    (1ul << 8)
#define CH32H41X_I2C_SR2_BUSY_S   (1ul << 9)
#define CH32H41X_I2C_SR2_TRA_S    (1ul << 10)
#define CH32H41X_I2C_SR2_GENCALL_S (1ul << 12)
#define CH32H41X_I2C_SR2_DUALF_S  (1ul << 14)

/* CKCFGR bits */

#define CH32H41X_I2C_CK_CCR_SHIFT 0              /* clock control (fast: 11:0) */
#define CH32H41X_I2C_CK_CCR_FAST_MASK (0xffful << CH32H41X_I2C_CK_CCR_SHIFT)
#define CH32H41X_I2C_CK_DUTY     (1ul << 14)     /* fast mode duty cycle */
#define CH32H41X_I2C_CK_FS       (1ul << 15)     /* fast mode selection */

/* RTR bits */

#define CH32H41X_I2C_RTR_RISE_SHIFT 0            /* max rise time (freq+1) */
#define CH32H41X_I2C_RTR_RISE_MASK (0x3ful << CH32H41X_I2C_RTR_RISE_SHIFT)

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_I2C_H */
