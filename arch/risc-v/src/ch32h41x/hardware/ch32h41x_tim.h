/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_tim.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_TIM_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_TIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses (16-bit registers on 32-bit stride; TIM9-12 expose
 * 32-bit CNT/ATRLR/CHxCVR at the same offsets). */

#define CH32H41X_TIM_CTLR1(t)     ((t) + 0x00)  /* control 1 */
#define CH32H41X_TIM_CTLR2(t)     ((t) + 0x04)  /* control 2 */
#define CH32H41X_TIM_SMCFGR(t)    ((t) + 0x08)  /* slave mode control */
#define CH32H41X_TIM_DMAINTENR(t) ((t) + 0x0c)  /* DMA/interrupt enable */
#define CH32H41X_TIM_INTFR(t)     ((t) + 0x10)  /* interrupt status */
#define CH32H41X_TIM_SWEVGR(t)    ((t) + 0x14)  /* software event generator */
#define CH32H41X_TIM_CHCTLR1(t)   ((t) + 0x18)  /* capture/compare control 1 */
#define CH32H41X_TIM_CHCTLR2(t)   ((t) + 0x1c)  /* capture/compare control 2 */
#define CH32H41X_TIM_CCER(t)      ((t) + 0x20)  /* capture/compare enable */
#define CH32H41X_TIM_CNT(t)       ((t) + 0x24)  /* counter */
#define CH32H41X_TIM_PSC(t)       ((t) + 0x28)  /* prescaler */
#define CH32H41X_TIM_ATRLR(t)     ((t) + 0x2c)  /* auto-reload */
#define CH32H41X_TIM_RPTCR(t)     ((t) + 0x30)  /* repetition counter */
#define CH32H41X_TIM_CH1CVR(t)    ((t) + 0x34)  /* capture/compare 1 */
#define CH32H41X_TIM_CH2CVR(t)    ((t) + 0x38)  /* capture/compare 2 */
#define CH32H41X_TIM_CH3CVR(t)    ((t) + 0x3c)  /* capture/compare 3 */
#define CH32H41X_TIM_CH4CVR(t)    ((t) + 0x40)  /* capture/compare 4 */
#define CH32H41X_TIM_BDTR(t)      ((t) + 0x44)  /* break/dead-time (TIM1/8) */
#define CH32H41X_TIM_DMACFGR(t)   ((t) + 0x48)  /* DMA control */
#define CH32H41X_TIM_DMAADR(t)    ((t) + 0x4c)  /* DMA address for burst mode */

/* CTLR1 bits */

#define CH32H41X_TIM_CR1_CEN      (1ul << 0)    /* counter enable */
#define CH32H41X_TIM_CR1_UDIS     (1ul << 1)    /* update disable */
#define CH32H41X_TIM_CR1_URS      (1ul << 2)    /* update request source */
#define CH32H41X_TIM_CR1_OPM      (1ul << 3)    /* one-pulse mode */
#define CH32H41X_TIM_CR1_DIR      (1ul << 4)    /* direction (0=up) */
#define CH32H41X_TIM_CR1_CMS_SHIFT 5            /* center-aligned mode */
#define CH32H41X_TIM_CR1_ARPE     (1ul << 7)    /* auto-reload preload */
#define CH32H41X_TIM_CR1_CKD_SHIFT 8            /* clock division (tDTS) */
#define CH32H41X_TIM_CR1_CKD_MASK (3ul << CH32H41X_TIM_CR1_CKD_SHIFT)

/* CTLR2 bits */

#define CH32H41X_TIM_CR2_CCPC     (1ul << 0)    /* CCx preload control */
#define CH32H41X_TIM_CR2_MMS_SHIFT 4            /* master mode selection */
#define CH32H41X_TIM_CR2_MMS_MASK (7ul << CH32H41X_TIM_CR2_MMS_SHIFT)
#define CH32H41X_TIM_CR2_MMS_RESET (0x0ul << CH32H41X_TIM_CR2_MMS_SHIFT)
#define CH32H41X_TIM_CR2_MMS_ENABLE (0x1ul << CH32H41X_TIM_CR2_MMS_SHIFT)
#define CH32H41X_TIM_CR2_MMS_UPDATE (0x2ul << CH32H41X_TIM_CR2_MMS_SHIFT)
#define CH32H41X_TIM_CR2_TI1S     (1ul << 7)    /* TI1 selection (xor) */
#define CH32H41X_TIM_CR2_OIS1     (1ul << 8)    /* output idle state 1 */
#define CH32H41X_TIM_CR2_OIS1N    (1ul << 9)    /* output idle state 1N */
#define CH32H41X_TIM_CR2_OIS2     (1ul << 10)
#define CH32H41X_TIM_CR2_OIS2N    (1ul << 11)
#define CH32H41X_TIM_CR2_OIS3     (1ul << 12)
#define CH32H41X_TIM_CR2_OIS3N    (1ul << 13)
#define CH32H41X_TIM_CR2_OIS4     (1ul << 14)

/* SMCFGR bits */

#define CH32H41X_TIM_SM_SMS_SHIFT 0              /* slave mode selection */
#define CH32H41X_TIM_SM_SMS_MASK  (7ul << CH32H41X_TIM_SM_SMS_SHIFT)
#define CH32H41X_TIM_SM_SMS_RESET (0x0ul << CH32H41X_TIM_SM_SMS_SHIFT)
#define CH32H41X_TIM_SM_SMS_ECE1  (0x7ul << CH32H41X_TIM_SM_SMS_SHIFT)  /* ext clock mode 1 */
#define CH32H41X_TIM_SM_TS_SHIFT  4              /* trigger selection */
#define CH32H41X_TIM_SM_TS_MASK   (7ul << CH32H41X_TIM_SM_TS_SHIFT)
#define CH32H41X_TIM_SM_MSMF      (1ul << 7)     /* master/slave mode */
#define CH32H41X_TIM_SM_ETF_SHIFT 8
#define CH32H41X_TIM_SM_ETF_MASK  (0xful << CH32H41X_TIM_SM_ETF_SHIFT)
#define CH32H41X_TIM_SM_ETPS_SHIFT 12
#define CH32H41X_TIM_SM_ETPS_MASK (3ul << CH32H41X_TIM_SM_ETPS_SHIFT)
#define CH32H41X_TIM_SM_ECE       (1ul << 14)    /* external clock enable */
#define CH32H41X_TIM_SM_ETP       (1ul << 15)    /* external trigger polarity */

/* DMAINTENR bits */

#define CH32H41X_TIM_DIER_UIE     (1ul << 0)     /* update interrupt enable */
#define CH32H41X_TIM_DIER_CC1IE   (1ul << 1)
#define CH32H41X_TIM_DIER_CC2IE   (1ul << 2)
#define CH32H41X_TIM_DIER_CC3IE   (1ul << 3)
#define CH32H41X_TIM_DIER_CC4IE   (1ul << 4)
#define CH32H41X_TIM_DIER_COMIE   (1ul << 5)     /* COM interrupt enable */
#define CH32H41X_TIM_DIER_TIE     (1ul << 6)     /* trigger interrupt enable */
#define CH32H41X_TIM_DIER_BIE     (1ul << 7)     /* break interrupt enable */
#define CH32H41X_TIM_DIER_UDE     (1ul << 8)     /* update DMA enable */
#define CH32H41X_TIM_DIER_CC1DE   (1ul << 9)
#define CH32H41X_TIM_DIER_CC2DE   (1ul << 10)
#define CH32H41X_TIM_DIER_CC3DE   (1ul << 11)
#define CH32H41X_TIM_DIER_CC4DE   (1ul << 12)
#define CH32H41X_TIM_DIER_COMDE   (1ul << 13)
#define CH32H41X_TIM_DIER_TDE     (1ul << 14)    /* trigger DMA enable */

/* INTFR bits (write 0 to clear) */

#define CH32H41X_TIM_INT_UIF      (1ul << 0)     /* update interrupt flag */
#define CH32H41X_TIM_INT_CC1IF    (1ul << 1)
#define CH32H41X_TIM_INT_CC2IF    (1ul << 2)
#define CH32H41X_TIM_INT_CC3IF    (1ul << 3)
#define CH32H41X_TIM_INT_CC4IF    (1ul << 4)
#define CH32H41X_TIM_INT_COMIF    (1ul << 5)
#define CH32H41X_TIM_INT_TIF      (1ul << 6)
#define CH32H41X_TIM_INT_BIF      (1ul << 7)
#define CH32H41X_TIM_INT_CC1OF    (1ul << 9)     /* capture 1 overcapture */
#define CH32H41X_TIM_INT_CC2OF    (1ul << 10)
#define CH32H41X_TIM_INT_CC3OF    (1ul << 11)
#define CH32H41X_TIM_INT_CC4OF    (1ul << 12)

/* SWEVGR bits (write 1 to generate) */

#define CH32H41X_TIM_EVG_UG       (1ul << 0)     /* update generation */
#define CH32H41X_TIM_EVG_CC1G     (1ul << 1)
#define CH32H41X_TIM_EVG_CC2G     (1ul << 2)
#define CH32H41X_TIM_EVG_CC3G     (1ul << 3)
#define CH32H41X_TIM_EVG_CC4G     (1ul << 4)
#define CH32H41X_TIM_EVG_COMG     (1ul << 5)
#define CH32H41X_TIM_EVG_TRGIG    (1ul << 6)
#define CH32H41X_TIM_EVG_BG       (1ul << 7)

/* CHCTLR1 bits (channel 1 low half, channel 2 high half) */

#define CH32H41X_TIM_CCMR_CC1S_SHIFT 0            /* CC1 channel selection */
#define CH32H41X_TIM_CCMR_CC1S_MASK  (3ul << CH32H41X_TIM_CCMR_CC1S_SHIFT)
#define CH32H41X_TIM_CCMR_CC1S_OUT   (0x0ul << CH32H41X_TIM_CCMR_CC1S_SHIFT)
#define CH32H41X_TIM_CCMR_CC1S_IN_TI1 (0x1ul << CH32H41X_TIM_CCMR_CC1S_SHIFT)
#define CH32H41X_TIM_CCMR_CC1S_IN_TI2 (0x2ul << CH32H41X_TIM_CCMR_CC1S_SHIFT)
#define CH32H41X_TIM_CCMR_OC1FE   (1ul << 2)      /* output compare 1 fast enable */
#define CH32H41X_TIM_CCMR_OC1PE   (1ul << 3)      /* output compare 1 preload */
#define CH32H41X_TIM_CCMR_OC1M_SHIFT 4            /* output compare 1 mode */
#define CH32H41X_TIM_CCMR_OC1M_MASK  (7ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_TIMING (0x0ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_ACTIVE (0x1ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_INACTIVE (0x2ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_TOGGLE (0x3ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_FORCED_INACTIVE (0x4ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_FORCED_ACTIVE (0x5ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_PWM1 (0x6ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1M_PWM2 (0x7ul << CH32H41X_TIM_CCMR_OC1M_SHIFT)
#define CH32H41X_TIM_CCMR_OC1CE   (1ul << 7)      /* output compare 1 clear enable */
#define CH32H41X_TIM_CCMR_IC1PSC_SHIFT 2          /* input capture 1 prescaler */
#define CH32H41X_TIM_CCMR_IC1F_SHIFT 4            /* input capture 1 filter */
#define CH32H41X_TIM_CCMR_IC1F_MASK  (0xful << CH32H41X_TIM_CCMR_IC1F_SHIFT)

#define CH32H41X_TIM_CCMR_CC2S_SHIFT 8
#define CH32H41X_TIM_CCMR_CC2S_MASK  (3ul << CH32H41X_TIM_CCMR_CC2S_SHIFT)
#define CH32H41X_TIM_CCMR_CC2S_OUT   (0x0ul << CH32H41X_TIM_CCMR_CC2S_SHIFT)
#define CH32H41X_TIM_CCMR_CC2S_IN_TI2 (0x1ul << CH32H41X_TIM_CCMR_CC2S_SHIFT)
#define CH32H41X_TIM_CCMR_CC2S_IN_TI1 (0x2ul << CH32H41X_TIM_CCMR_CC2S_SHIFT)
#define CH32H41X_TIM_CCMR_OC2FE   (1ul << 10)
#define CH32H41X_TIM_CCMR_OC2PE   (1ul << 11)
#define CH32H41X_TIM_CCMR_OC2M_SHIFT 12
#define CH32H41X_TIM_CCMR_OC2M_MASK  (7ul << CH32H41X_TIM_CCMR_OC2M_SHIFT)
#define CH32H41X_TIM_CCMR_OC2M_PWM1 (0x6ul << CH32H41X_TIM_CCMR_OC2M_SHIFT)
#define CH32H41X_TIM_CCMR_OC2M_PWM2 (0x7ul << CH32H41X_TIM_CCMR_OC2M_SHIFT)
#define CH32H41X_TIM_CCMR_OC2CE   (1ul << 15)
#define CH32H41X_TIM_CCMR_IC2PSC_SHIFT 10
#define CH32H41X_TIM_CCMR_IC2F_SHIFT 12
#define CH32H41X_TIM_CCMR_IC2F_MASK  (0xful << CH32H41X_TIM_CCMR_IC2F_SHIFT)

/* CHCTLR2 bits (channel 3 low half, channel 4 high half; same layout) */

#define CH32H41X_TIM_CCMR_CC3S_SHIFT 0
#define CH32H41X_TIM_CCMR_CC3S_MASK  (3ul << CH32H41X_TIM_CCMR_CC3S_SHIFT)
#define CH32H41X_TIM_CCMR_CC3S_OUT   (0x0ul << CH32H41X_TIM_CCMR_CC3S_SHIFT)
#define CH32H41X_TIM_CCMR_CC3S_IN_TI3 (0x1ul << CH32H41X_TIM_CCMR_CC3S_SHIFT)
#define CH32H41X_TIM_CCMR_OC3PE   (1ul << 3)
#define CH32H41X_TIM_CCMR_OC3M_SHIFT 4
#define CH32H41X_TIM_CCMR_OC3M_MASK  (7ul << CH32H41X_TIM_CCMR_OC3M_SHIFT)
#define CH32H41X_TIM_CCMR_OC3M_PWM1 (0x6ul << CH32H41X_TIM_CCMR_OC3M_SHIFT)
#define CH32H41X_TIM_CCMR_OC3M_PWM2 (0x7ul << CH32H41X_TIM_CCMR_OC3M_SHIFT)

#define CH32H41X_TIM_CCMR_CC4S_SHIFT 8
#define CH32H41X_TIM_CCMR_CC4S_MASK  (3ul << CH32H41X_TIM_CCMR_CC4S_SHIFT)
#define CH32H41X_TIM_CCMR_CC4S_OUT   (0x0ul << CH32H41X_TIM_CCMR_CC4S_SHIFT)
#define CH32H41X_TIM_CCMR_CC4S_IN_TI4 (0x1ul << CH32H41X_TIM_CCMR_CC4S_SHIFT)
#define CH32H41X_TIM_CCMR_OC4PE   (1ul << 11)
#define CH32H41X_TIM_CCMR_OC4M_SHIFT 12
#define CH32H41X_TIM_CCMR_OC4M_MASK  (7ul << CH32H41X_TIM_CCMR_OC4M_SHIFT)
#define CH32H41X_TIM_CCMR_OC4M_PWM1 (0x6ul << CH32H41X_TIM_CCMR_OC4M_SHIFT)
#define CH32H41X_TIM_CCMR_OC4M_PWM2 (0x7ul << CH32H41X_TIM_CCMR_OC4M_SHIFT)
#define CH32H41X_TIM_CCMR_IC4F_SHIFT 12
#define CH32H41X_TIM_CCMR_IC4F_MASK  (0xful << CH32H41X_TIM_CCMR_IC4F_SHIFT)

/* CCER bits (per channel: E, P, NE, NP on 4-bit boundaries) */

#define CH32H41X_TIM_CCE_CC1E     (1ul << 0)      /* capture/compare 1 enable */
#define CH32H41X_TIM_CCE_CC1P     (1ul << 1)      /* capture/compare 1 polarity */
#define CH32H41X_TIM_CCE_CC1NE    (1ul << 2)      /* complementary 1 enable */
#define CH32H41X_TIM_CCE_CC1NP    (1ul << 3)      /* complementary 1 polarity */
#define CH32H41X_TIM_CCE_CC2E     (1ul << 4)
#define CH32H41X_TIM_CCE_CC2P     (1ul << 5)
#define CH32H41X_TIM_CCE_CC2NE    (1ul << 6)
#define CH32H41X_TIM_CCE_CC2NP    (1ul << 7)
#define CH32H41X_TIM_CCE_CC3E     (1ul << 8)
#define CH32H41X_TIM_CCE_CC3P     (1ul << 9)
#define CH32H41X_TIM_CCE_CC3NE    (1ul << 10)
#define CH32H41X_TIM_CCE_CC3NP    (1ul << 11)
#define CH32H41X_TIM_CCE_CC4E     (1ul << 12)
#define CH32H41X_TIM_CCE_CC4P     (1ul << 13)

/* BDTR bits (TIM1/TIM8 only) */

#define CH32H41X_TIM_BD_DTG_SHIFT 0               /* dead-time generator */
#define CH32H41X_TIM_BD_DTG_MASK  (0xful << CH32H41X_TIM_BD_DTG_SHIFT)
#define CH32H41X_TIM_BD_LOCK_SHIFT 8              /* lock configuration */
#define CH32H41X_TIM_BD_LOCK_MASK (3ul << CH32H41X_TIM_BD_LOCK_SHIFT)
#define CH32H41X_TIM_BD_OSSI      (1ul << 10)     /* off-state selection idle */
#define CH32H41X_TIM_BD_OSSR      (1ul << 11)     /* off-state selection run */
#define CH32H41X_TIM_BD_BKE       (1ul << 12)     /* break enable */
#define CH32H41X_TIM_BD_BKP       (1ul << 13)     /* break polarity */
#define CH32H41X_TIM_BD_AOE       (1ul << 14)     /* automatic output enable */
#define CH32H41X_TIM_BD_MOE       (1ul << 15)     /* main output enable */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_TIM_H */
