/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/include/board.h
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

#ifndef __BOARDS_RISC_V_CH32H41X_CH32H41X_EVB_INCLUDE_BOARD_H
#define __BOARDS_RISC_V_CH32H41X_CH32H41X_EVB_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clock sources
 *
 * The evaluation board carries a 25MHz crystal on HSE.  Clock tree:
 * V5F core = SYSCLK/HPRE; HCLK = SYSCLK/HPRE/FPRE; V3F core = HCLK;
 * peripherals = HCLK.
 *
 * With CONFIG_CH32H41X_SYSCLK_480MHZ (default): SYSCLK = 480MHz,
 * V5F core = 240MHz, HCLK = V3F core = peripherals = 120MHz.
 * With CONFIG_CH32H41X_SYSCLK_400MHZ: SYSCLK = 400MHz, V5F core =
 * 400MHz, HCLK = V3F core = peripherals = 100MHz.
 */

#define CH32H41X_EVB_HSE_CRYSTAL 25000000

/* Console USART1
 *
 * USART1 is wired to the on-board USB-serial adapter:
 *   PA9  = USART1 TX (AF7)
 *   PA10 = USART1 RX (AF7)
 *
 * Port numbers follow the ch32h41x_gpio.c port indexing
 * (0 = GPIOA .. 5 = GPIOF).
 */

#define GPIO_USART1_TX_PORT 0         /* GPIOA */
#define GPIO_USART1_TX_PIN  9
#define GPIO_USART1_TX_AF   7

#define GPIO_USART1_RX_PORT 0         /* GPIOA */
#define GPIO_USART1_RX_PIN  10
#define GPIO_USART1_RX_AF   7

/* Loopback / self-test pin assignments.  Port numbers follow the
 * ch32h41x_gpio.c port indexing (0 = GPIOA .. 5 = GPIOF).  All pin/AF
 * combinations are taken from the WCH EVT example programs and the
 * CH32H417 datasheet (CH32H417DS0) pin alternate-function table.
 *
 * Loopback jumpers for the driver self-tests:
 *   PE11 (TIM1_CH2 PWM out) -> PA0  (TIM2_CH1 capture in)
 *   PA5  (TIM1_CH1 out)     -> PA1  (ADC1_IN1 sample input)
 *   PB13 (SPI2 SCK)         -> tied locally;  PC1 MOSI -> PC2 MISO
 *   PB6  (I2C1 SCL)         -> PC0  (I2C2 SCL); PB7 (SDA) -> PC1 (SDA)
 */

/* TIM1 CH1 output on PA8 (datasheet AF table; PA8 AF1 = TIM1_CH1).
 * (PA5/AF3 is TIM8_CH1N, not TIM1_CH1 - the earlier EVT-demo-derived
 * PA5 assignment was wrong and produced no output.) */

#define GPIO_TIM1_CH1_PORT  0         /* GPIOA */
#define GPIO_TIM1_CH1_PIN   8
#define GPIO_TIM1_CH1_AF    1

/* TIM1 CH1N complementary output on PC6 (EVT ComplementaryOutput demo) */

#define GPIO_TIM1_CH1N_PORT 2         /* GPIOC */
#define GPIO_TIM1_CH1N_PIN  6
#define GPIO_TIM1_CH1N_AF   3

/* TIM1 CH2 PWM output (EVT PWM_Output) */

#define GPIO_TIM1_CH2_PORT  4         /* GPIOE */
#define GPIO_TIM1_CH2_PIN   11
#define GPIO_TIM1_CH2_AF    1

/* TIM2 CH2 capture input on PA1 (datasheet AF table; PA1 AF1 = TIM2_CH2).
 * The loopback jumper PA8(TIM1_CH1 PWM) -> PA0 feeds this pin. */

#define GPIO_TIM2_CAP_PORT  0         /* GPIOA */
#define GPIO_TIM2_CAP_PIN   0
#define GPIO_TIM2_CAP_AF    1

/* ADC1 external sample input */

#define GPIO_ADC1_IN1_PORT  0         /* GPIOA */
#define GPIO_ADC1_IN1_PIN   1         /* PA1 = ADC1_IN1 */

/* SPI2 (EVT 2Lines_FullDuplex) */

#define GPIO_SPI2_SCK_PORT  1         /* GPIOB */
#define GPIO_SPI2_SCK_PIN   13
#define GPIO_SPI2_SCK_AF    5

#define GPIO_SPI2_MOSI_PORT 2         /* GPIOC */
#define GPIO_SPI2_MOSI_PIN  1
#define GPIO_SPI2_MOSI_AF   5

#define GPIO_SPI2_MISO_PORT 2         /* GPIOC */
#define GPIO_SPI2_MISO_PIN  2
#define GPIO_SPI2_MISO_AF   5

/* I2C1 (datasheet AF table) - used as the master in the loopback test */

#define GPIO_I2C1_SCL_PORT  1         /* GPIOB */
#define GPIO_I2C1_SCL_PIN   6
#define GPIO_I2C1_SCL_AF    4

#define GPIO_I2C1_SDA_PORT  1         /* GPIOB */
#define GPIO_I2C1_SDA_PIN   7
#define GPIO_I2C1_SDA_AF    4

/* I2C2 (EVT I2C_7bit_Mode) - used as the slave in the loopback test */

#define GPIO_I2C2_SCL_PORT  2         /* GPIOC */
#define GPIO_I2C2_SCL_PIN   0
#define GPIO_I2C2_SCL_AF    9

#define GPIO_I2C2_SDA_PORT  2         /* GPIOC */
#define GPIO_I2C2_SDA_PIN   1
#define GPIO_I2C2_SDA_AF    9

/* CAN1 (EVT CAN Networking; pins unused in loopback mode) */

#define GPIO_CAN1_TX_PORT   0         /* GPIOA */
#define GPIO_CAN1_TX_PIN    12
#define GPIO_CAN1_TX_AF     9

#define GPIO_CAN1_RX_PORT   0         /* GPIOA */
#define GPIO_CAN1_RX_PIN    11
#define GPIO_CAN1_RX_AF     9

#endif /* __BOARDS_RISC_V_CH32H41X_CH32H41X_EVB_INCLUDE_BOARD_H */
