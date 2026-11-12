/****************************************************************************
 * arch/risc-v/src/ch32h41x/hardware/ch32h41x_adc.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_ADC_H
#define __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_ADC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ch32h41x_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register addresses (32-bit registers, dense layout per EVT ADC_TypeDef).
 * AUX/DRV registers at 0x54/0x58 are WCH-specific. */

#define CH32H41X_ADC_STATR(a)    ((a) + 0x00)   /* status */
#define CH32H41X_ADC_CTLR1(a)    ((a) + 0x04)   /* control 1 */
#define CH32H41X_ADC_CTLR2(a)    ((a) + 0x08)   /* control 2 */
#define CH32H41X_ADC_SAMPTR1(a)  ((a) + 0x0c)   /* sample time 1 (ch 0-7) */
#define CH32H41X_ADC_SAMPTR2(a)  ((a) + 0x10)   /* sample time 2 (ch 8-9) */
#define CH32H41X_ADC_IOFR1(a)    ((a) + 0x14)   /* injected offset 1 */
#define CH32H41X_ADC_IOFR2(a)    ((a) + 0x18)
#define CH32H41X_ADC_IOFR3(a)    ((a) + 0x1c)
#define CH32H41X_ADC_IOFR4(a)    ((a) + 0x20)
#define CH32H41X_ADC_WDHTR(a)    ((a) + 0x24)   /* watchdog high threshold */
#define CH32H41X_ADC_WDLTR(a)    ((a) + 0x28)   /* watchdog low threshold */
#define CH32H41X_ADC_RSQR1(a)    ((a) + 0x2c)   /* regular sequence 1 */
#define CH32H41X_ADC_RSQR2(a)    ((a) + 0x30)
#define CH32H41X_ADC_RSQR3(a)    ((a) + 0x34)
#define CH32H41X_ADC_ISQR(a)     ((a) + 0x38)   /* injected sequence */
#define CH32H41X_ADC_IDATAR1(a)  ((a) + 0x3c)   /* injected data 1 */
#define CH32H41X_ADC_IDATAR2(a)  ((a) + 0x40)
#define CH32H41X_ADC_IDATAR3(a)  ((a) + 0x44)
#define CH32H41X_ADC_IDATAR4(a)  ((a) + 0x48)
#define CH32H41X_ADC_RDATAR(a)   ((a) + 0x4c)   /* regular data */
#define CH32H41X_ADC_AUX(a)      ((a) + 0x54)   /* per-channel SMP mode 0/1 select */
#define CH32H41X_ADC_DRV(a)      ((a) + 0x58)   /* drive strength */

/* STATR bits */

#define CH32H41X_ADC_SR_AWD      (1ul << 0)     /* analog watchdog flag */
#define CH32H41X_ADC_SR_EOC      (1ul << 1)     /* end of conversion */
#define CH32H41X_ADC_SR_JEOC     (1ul << 2)     /* injected end of conversion */
#define CH32H41X_ADC_SR_JSTRT    (1ul << 3)     /* injected start */
#define CH32H41X_ADC_SR_STRT     (1ul << 4)     /* regular start */
#define CH32H41X_ADC_SR_RSTF     (1ul << 15)    /* reset flag */

/* CTLR1 bits */

#define CH32H41X_ADC_CR1_AWDCH_SHIFT 0          /* watchdog channel */
#define CH32H41X_ADC_CR1_AWDCH_MASK (0x1ful << CH32H41X_ADC_CR1_AWDCH_SHIFT)
#define CH32H41X_ADC_CR1_EOCIE   (1ul << 5)     /* EOC interrupt enable */
#define CH32H41X_ADC_CR1_AWDIE   (1ul << 6)     /* watchdog interrupt enable */
#define CH32H41X_ADC_CR1_JEOCIE  (1ul << 7)     /* injected EOC int enable */
#define CH32H41X_ADC_CR1_SCAN    (1ul << 8)     /* scan mode */
#define CH32H41X_ADC_CR1_AWDSGL  (1ul << 9)     /* watchdog single channel */
#define CH32H41X_ADC_CR1_JAUTO   (1ul << 10)    /* automatic injected group */
#define CH32H41X_ADC_CR1_DISCEN  (1ul << 11)    /* discontinuous regular */
#define CH32H41X_ADC_CR1_JDISCEN (1ul << 12)    /* discontinuous injected */
#define CH32H41X_ADC_CR1_DISCNUM_SHIFT 13       /* discontinuous channel count - 1 */
#define CH32H41X_ADC_CR1_DISCNUM_MASK (7ul << CH32H41X_ADC_CR1_DISCNUM_SHIFT)
#define CH32H41X_ADC_CR1_DUALMOD_SHIFT 16       /* dual mode selection */
#define CH32H41X_ADC_CR1_DUALMOD_MASK (0xful << CH32H41X_ADC_CR1_DUALMOD_SHIFT)
#define CH32H41X_ADC_CR1_JAWDEN  (1ul << 22)    /* watchdog on injected */
#define CH32H41X_ADC_CR1_AWDEN   (1ul << 23)    /* watchdog on regular */
#define CH32H41X_ADC_CR1_BUFEN   (1ul << 26)    /* output buffer (needed
                                                 * for the internal channels) */
#define CH32H41X_ADC_CR1_ANA_RST (1ul << 30)    /* analog front-end reset */
#define CH32H41X_ADC_CR1_SW_PRE  (1ul << 31)    /* software preselection */

/* CTLR2 bits */

#define CH32H41X_ADC_CR2_ADON    (1ul << 0)     /* A/D converter on */
#define CH32H41X_ADC_CR2_CONT    (1ul << 1)     /* continuous conversion */
#define CH32H41X_ADC_CR2_CAL     (1ul << 2)     /* calibration */
#define CH32H41X_ADC_CR2_RSTCAL  (1ul << 3)     /* reset calibration */
#define CH32H41X_ADC_CR2_DMA     (1ul << 8)     /* direct memory access mode */
#define CH32H41X_ADC_CR2_ALIGN   (1ul << 11)    /* data alignment (1=left) */
#define CH32H41X_ADC_CR2_JEXTSEL_SHIFT 12       /* injected external event */
#define CH32H41X_ADC_CR2_JEXTSEL_MASK (7ul << CH32H41X_ADC_CR2_JEXTSEL_SHIFT)
#define CH32H41X_ADC_CR2_JEXTTRIG (1ul << 15)   /* injected ext trigger mode */
#define CH32H41X_ADC_CR2_EXTSEL_SHIFT 17        /* regular external event */
#define CH32H41X_ADC_CR2_EXTSEL_MASK (7ul << CH32H41X_ADC_CR2_EXTSEL_SHIFT)
#define CH32H41X_ADC_CR2_EXTTRIG (1ul << 20)    /* regular ext trigger mode */
#define CH32H41X_ADC_CR2_JSWSTART (1ul << 21)   /* start injected conversion */
#define CH32H41X_ADC_CR2_SWSTART (1ul << 22)    /* start regular conversion */
#define CH32H41X_ADC_CR2_TSVREFE (1ul << 23)    /* temp sensor / VREFINT enable */

/* SAMPTR1/2: 3 bits per channel (SMP=000 -> 1.5clks; 001 -> 7.5;
 * 010 -> 13.5; 011 -> 28.5; 100 -> 41.5; 101 -> 55.5; 110 -> 71.5;
 * 111 -> 239.5).  NOTE: swapped vs STM32F1 - on this chip
 * ADC_RegularChannelConfig() puts channels 10..17 into SAMPTR1 (at
 * 3*(ch-10)) and channels 0..9 into SAMPTR2 (at 3*ch). */

#define CH32H41X_ADC_SMP_SHIFT(n) (3ul * (n))
#define CH32H41X_ADC_SMP_MASK(n)  (7ul << CH32H41X_ADC_SMP_SHIFT(n))
#define CH32H41X_ADC_SMP_1P5      0x0ul
#define CH32H41X_ADC_SMP_7P5      0x1ul
#define CH32H41X_ADC_SMP_13P5     0x2ul
#define CH32H41X_ADC_SMP_28P5     0x3ul
#define CH32H41X_ADC_SMP_41P5     0x4ul
#define CH32H41X_ADC_SMP_55P5     0x5ul
#define CH32H41X_ADC_SMP_71P5     0x6ul
#define CH32H41X_ADC_SMP_239P5    0x7ul

/* IOFRn: injected offset, bits 11:0 */

#define CH32H41X_ADC_IOFR_MASK   0xffful

/* RSQR1: regular sequence length L[3:0] at bits 23:20 (=L-1) and first
 * conversions SQ13..SQ16;  RSQR2: SQ7..SQ12;  RSQR3: SQ1..SQ6 (5 bits
 * each, SQ1 at bits 4:0). */

#define CH32H41X_ADC_RSQR1_L_SHIFT 20
#define CH32H41X_ADC_RSQR1_L_MASK (0xful << CH32H41X_ADC_RSQR1_L_SHIFT)
#define CH32H41X_ADC_RSQR3_SQ1_SHIFT 0
#define CH32H41X_ADC_RSQR3_SQ1_MASK (0x1ful << CH32H41X_ADC_RSQR3_SQ1_SHIFT)
#define CH32H41X_ADC_RSQR_SQ(n)   (5ul * ((n) - 1))

/* ISQR: injected sequence length JL[1:0] at bits 21:20, JSQ at 5-bit
 * fields from bits 15:0. */

#define CH32H41X_ADC_ISQR_JL_SHIFT 20
#define CH32H41X_ADC_ISQR_JL_MASK (3ul << CH32H41X_ADC_ISQR_JL_SHIFT)

/* Channel numbers for internal sources */

#define CH32H41X_ADC_CHAN_TSENSE  16            /* temperature sensor */
#define CH32H41X_ADC_CHAN_VREFINT 17            /* internal reference */

#endif /* __ARCH_RISCV_SRC_CH32H41X_HARDWARE_CH32H41X_ADC_H */
