/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_can.h
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
 * bxCAN-style controller (CAN1/CAN2/CAN3).  Register layout and bit values
 * cross-checked against the WCH EVT ch32h417.h bit-definition blocks.
 *
 ****************************************************************************/

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_CAN_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_CAN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses */

#define CH32H41X_CAN_CTLR(c)     ((c) + 0x00)
#define CH32H41X_CAN_STATR(c)    ((c) + 0x04)
#define CH32H41X_CAN_TSTATR(c)   ((c) + 0x08)
#define CH32H41X_CAN_RFIFO0(c)   ((c) + 0x0c)
#define CH32H41X_CAN_RFIFO1(c)   ((c) + 0x10)
#define CH32H41X_CAN_INTENR(c)   ((c) + 0x14)
#define CH32H41X_CAN_ERRSR(c)    ((c) + 0x18)
#define CH32H41X_CAN_BTIMR(c)    ((c) + 0x1c)

/* Tx mailbox n (n = 0..2), 16 bytes each, block base 0x180 */

#define CH32H41X_CAN_TXMIR(c, n)  ((c) + 0x180 + (n) * 16 + 0x00)
#define CH32H41X_CAN_TXMDTR(c, n) ((c) + 0x180 + (n) * 16 + 0x04)
#define CH32H41X_CAN_TXMDLR(c, n) ((c) + 0x180 + (n) * 16 + 0x08)
#define CH32H41X_CAN_TXMDHR(c, n) ((c) + 0x180 + (n) * 16 + 0x0c)

/* Rx FIFO mailbox n (n = 0..1), 16 bytes each, block base 0x1b0 */

#define CH32H41X_CAN_RXMIR(c, n)  ((c) + 0x1b0 + (n) * 16 + 0x00)
#define CH32H41X_CAN_RXMDTR(c, n) ((c) + 0x1b0 + (n) * 16 + 0x04)
#define CH32H41X_CAN_RXMDLR(c, n) ((c) + 0x1b0 + (n) * 16 + 0x08)
#define CH32H41X_CAN_RXMDHR(c, n) ((c) + 0x1b0 + (n) * 16 + 0x0c)

/* Filter registers (base 0x200).  CAN1/CAN2 share the primary filter set;
 * CAN3 uses the _CAN3 shadow registers at +4. */

#define CH32H41X_CAN_FCTLR(c)        ((c) + 0x200)
#define CH32H41X_CAN_FMCFGR(c)       ((c) + 0x204)
#define CH32H41X_CAN_FMCFGR_CAN3(c)  ((c) + 0x208)
#define CH32H41X_CAN_FSCFGR(c)       ((c) + 0x20c)
#define CH32H41X_CAN_FSCFGR_CAN3(c)  ((c) + 0x210)
#define CH32H41X_CAN_FAFIFOR(c)      ((c) + 0x214)
#define CH32H41X_CAN_FAFIFOR_CAN3(c) ((c) + 0x218)
#define CH32H41X_CAN_FWR(c)          ((c) + 0x21c)
#define CH32H41X_CAN_FWR_CAN3(c)     ((c) + 0x220)
#define CH32H41X_CAN_FR(c, i)        ((c) + 0x240 + (i) * 8)  /* FR1 of bank i */
#define CH32H41X_CAN_FR2(c, i)       ((c) + 0x240 + (i) * 8 + 4)

/* CTLR bits */

#define CH32H41X_CAN_CR_INRQ      (1ul << 0)     /* initialization request */
#define CH32H41X_CAN_CR_SLEEP     (1ul << 1)     /* sleep mode request */
#define CH32H41X_CAN_CR_TXFP      (1ul << 2)     /* transmit FIFO priority */
#define CH32H41X_CAN_CR_RFLM      (1ul << 3)     /* receive FIFO locked mode */
#define CH32H41X_CAN_CR_NART      (1ul << 4)     /* no automatic retransmission */
#define CH32H41X_CAN_CR_AWUM      (1ul << 5)     /* automatic wakeup mode */
#define CH32H41X_CAN_CR_ABOM      (1ul << 6)     /* automatic bus-off management */
#define CH32H41X_CAN_CR_TTCM      (1ul << 7)     /* time triggered comm mode */
#define CH32H41X_CAN_CR_RESET     (1ul << 15)    /* software master reset */
#define CH32H41X_CAN_CR_DBF       (1ul << 16)    /* debug freeze */

/* STATR bits */

#define CH32H41X_CAN_ST_INAK      (1ul << 0)     /* initialization acknowledge */
#define CH32H41X_CAN_ST_SLAK      (1ul << 1)     /* sleep acknowledge */
#define CH32H41X_CAN_ST_ERRI      (1ul << 2)     /* error interrupt */
#define CH32H41X_CAN_ST_WKUI      (1ul << 3)     /* wakeup interrupt */
#define CH32H41X_CAN_ST_SLAKI     (1ul << 4)     /* sleep acknowledge interrupt */
#define CH32H41X_CAN_ST_TXM       (1ul << 8)     /* transmit mode */
#define CH32H41X_CAN_ST_RXM       (1ul << 9)     /* receive mode */
#define CH32H41X_CAN_ST_SAMP      (1ul << 10)    /* last sample point */
#define CH32H41X_CAN_ST_RX        (1ul << 11)    /* CAN rx pin level */

/* TSTATR bits (mailbox n at 16*n) */

#define CH32H41X_CAN_TS_RQCP0     (1ul << 0)     /* request completed mb0 (w1c) */
#define CH32H41X_CAN_TS_TXOK0     (1ul << 1)
#define CH32H41X_CAN_TS_ALST0     (1ul << 2)
#define CH32H41X_CAN_TS_TERR0     (1ul << 3)
#define CH32H41X_CAN_TS_ABRQ0     (1ul << 7)     /* abort request mb0 */
#define CH32H41X_CAN_TS_RQCP1     (1ul << 8)
#define CH32H41X_CAN_TS_TXOK1     (1ul << 9)
#define CH32H41X_CAN_TS_ALST1     (1ul << 10)
#define CH32H41X_CAN_TS_TERR1     (1ul << 11)
#define CH32H41X_CAN_TS_ABRQ1     (1ul << 15)
#define CH32H41X_CAN_TS_RQCP2     (1ul << 16)
#define CH32H41X_CAN_TS_TXOK2     (1ul << 17)
#define CH32H41X_CAN_TS_ALST2     (1ul << 18)
#define CH32H41X_CAN_TS_TERR2     (1ul << 19)
#define CH32H41X_CAN_TS_ABRQ2     (1ul << 23)
#define CH32H41X_CAN_TS_CODE_SHIFT 24            /* mailbox code */
#define CH32H41X_CAN_TS_TME_MASK  (7ul << 26)    /* transmit mailbox empty */
#define CH32H41X_CAN_TS_TME0      (1ul << 26)
#define CH32H41X_CAN_TS_TME1      (1ul << 27)
#define CH32H41X_CAN_TS_TME2      (1ul << 28)

/* RFIFO0/RFIFO1 bits */

#define CH32H41X_CAN_RF_FMP_MASK  0x3ul          /* message pending count */
#define CH32H41X_CAN_RF_FULL      (1ul << 3)     /* FIFO full */
#define CH32H41X_CAN_RF_FOVR      (1ul << 4)     /* FIFO overrun */
#define CH32H41X_CAN_RF_RFOM      (1ul << 5)     /* release output mailbox (w1c) */

/* INTENR bits */

#define CH32H41X_CAN_IR_TMEIE     (1ul << 0)     /* TX mailbox empty int */
#define CH32H41X_CAN_IR_FMPIE0    (1ul << 1)     /* FIFO0 message pending int */
#define CH32H41X_CAN_IR_FFIE0     (1ul << 2)
#define CH32H41X_CAN_IR_FOVIE0    (1ul << 3)
#define CH32H41X_CAN_IR_FMPIE1    (1ul << 4)     /* FIFO1 message pending int */
#define CH32H41X_CAN_IR_FFIE1     (1ul << 5)
#define CH32H41X_CAN_IR_FOVIE1    (1ul << 6)
#define CH32H41X_CAN_IR_EWGIE     (1ul << 8)
#define CH32H41X_CAN_IR_EPVIE     (1ul << 9)
#define CH32H41X_CAN_IR_BOFIE     (1ul << 10)
#define CH32H41X_CAN_IR_LECIE     (1ul << 11)
#define CH32H41X_CAN_IR_ERRIE     (1ul << 15)
#define CH32H41X_CAN_IR_WKUIE     (1ul << 16)
#define CH32H41X_CAN_IR_SLKIE     (1ul << 17)

/* ERRSR bits */

#define CH32H41X_CAN_ES_EWGF      (1ul << 0)
#define CH32H41X_CAN_ES_EPVF      (1ul << 1)
#define CH32H41X_CAN_ES_BOFF      (1ul << 2)
#define CH32H41X_CAN_ES_LEC_SHIFT 4              /* last error code */
#define CH32H41X_CAN_ES_LEC_MASK  (7ul << CH32H41X_CAN_ES_LEC_SHIFT)
#define CH32H41X_CAN_ES_TEC_SHIFT 16             /* tx error counter */
#define CH32H41X_CAN_ES_REC_SHIFT 24             /* rx error counter */

/* BTIMR bits (WCH layout: 10-bit prescaler) */

#define CH32H41X_CAN_BTR_BRP_SHIFT 0             /* baud rate prescaler-1 */
#define CH32H41X_CAN_BTR_BRP_MASK (0x3fful << CH32H41X_CAN_BTR_BRP_SHIFT)
#define CH32H41X_CAN_BTR_TS1_SHIFT 16            /* segment 1 - 1 */
#define CH32H41X_CAN_BTR_TS1_MASK (0xful << CH32H41X_CAN_BTR_TS1_SHIFT)
#define CH32H41X_CAN_BTR_TS2_SHIFT 20            /* segment 2 - 1 */
#define CH32H41X_CAN_BTR_TS2_MASK (0xful << CH32H41X_CAN_BTR_TS2_SHIFT)
#define CH32H41X_CAN_BTR_SJW_SHIFT 24            /* resync width - 1 */
#define CH32H41X_CAN_BTR_SJW_MASK (0xful << CH32H41X_CAN_BTR_SJW_SHIFT)
#define CH32H41X_CAN_BTR_LBKM     (1ul << 30)    /* loopback mode */
#define CH32H41X_CAN_BTR_SILM     (1ul << 31)    /* silent mode */

/* Filter master control */

#define CH32H41X_CAN_FM_FINITE    (1ul << 0)     /* filter init mode */

/* Mailbox identifier register bits */

#define CH32H41X_CAN_TI_TXRQ      (1ul << 0)     /* transmit mailbox request */
#define CH32H41X_CAN_TI_RTR       (1ul << 1)     /* remote transmission request */
#define CH32H41X_CAN_TI_IDE       (1ul << 2)     /* identifier extension */
#define CH32H41X_CAN_TI_EXID_SHIFT 3             /* extended id (bits 3..20) */
#define CH32H41X_CAN_TI_EXID_MASK (0x1fffful << CH32H41X_CAN_TI_EXID_SHIFT)
#define CH32H41X_CAN_TI_STID_SHIFT 21            /* standard id (bits 21..28) */
#define CH32H41X_CAN_TI_STID_MASK (0x7fful << CH32H41X_CAN_TI_STID_SHIFT)

/* Data length register bits */

#define CH32H41X_CAN_DT_DLC_SHIFT 0              /* data length code 0..8 */
#define CH32H41X_CAN_DT_DLC_MASK  (0xful << CH32H41X_CAN_DT_DLC_SHIFT)
#define CH32H41X_CAN_DT_TGT       (1ul << 8)     /* transmit global time */

/* Rx data length register extras (timestamps in upper half) */

#define CH32H41X_CAN_DT_TIME_SHIFT 16

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_CAN_H */
