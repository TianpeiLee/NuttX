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

#endif /* __BOARDS_RISC_V_CH32H41X_CH32H41X_EVB_INCLUDE_BOARD_H */
