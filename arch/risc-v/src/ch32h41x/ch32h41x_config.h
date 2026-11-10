/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_config.h
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

#ifndef __ARCH_RISCV_SRC_CH32H41X_CH32H41X_CONFIG_H
#define __ARCH_RISCV_SRC_CH32H41X_CH32H41X_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <arch/board/board.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Is there a serial console?  (NuttX names the console items UART<n>_*
 * even for USART peripherals.)
 */

#if defined(CONFIG_UART1_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART1)
#  define HAVE_USART1_CONSOLE 1
#elif defined(CONFIG_UART2_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART2)
#  define HAVE_USART2_CONSOLE 1
#elif defined(CONFIG_UART3_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART3)
#  define HAVE_USART3_CONSOLE 1
#elif defined(CONFIG_UART4_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART4)
#  define HAVE_USART4_CONSOLE 1
#elif defined(CONFIG_UART5_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART5)
#  define HAVE_USART5_CONSOLE 1
#elif defined(CONFIG_UART6_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART6)
#  define HAVE_USART6_CONSOLE 1
#elif defined(CONFIG_UART7_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART7)
#  define HAVE_USART7_CONSOLE 1
#elif defined(CONFIG_UART8_SERIAL_CONSOLE) && defined(CONFIG_CH32H41X_USART8)
#  define HAVE_USART8_CONSOLE 1
#endif

#ifdef HAVE_USART1_CONSOLE
#  define CONSOLE_UART 1
#  define HAVE_SERIAL_CONSOLE 1
#elif defined(HAVE_USART2_CONSOLE)
#  define CONSOLE_UART 2
#  define HAVE_SERIAL_CONSOLE 1
#elif defined(HAVE_USART3_CONSOLE)
#  define CONSOLE_UART 3
#  define HAVE_SERIAL_CONSOLE 1
#elif defined(HAVE_USART4_CONSOLE)
#  define CONSOLE_UART 4
#  define HAVE_SERIAL_CONSOLE 1
#elif defined(HAVE_USART5_CONSOLE)
#  define CONSOLE_UART 5
#  define HAVE_SERIAL_CONSOLE 1
#elif defined(HAVE_USART6_CONSOLE)
#  define CONSOLE_UART 6
#  define HAVE_SERIAL_CONSOLE 1
#elif defined(HAVE_USART7_CONSOLE)
#  define CONSOLE_UART 7
#  define HAVE_SERIAL_CONSOLE 1
#elif defined(HAVE_USART8_CONSOLE)
#  define CONSOLE_UART 8
#  define HAVE_SERIAL_CONSOLE 1
#else
#  undef CONSOLE_UART
#  undef HAVE_SERIAL_CONSOLE
#endif

/* Is there *any* UART enabled? */

#if defined(CONFIG_CH32H41X_USART1) || defined(CONFIG_CH32H41X_USART2) || \
    defined(CONFIG_CH32H41X_USART3) || defined(CONFIG_CH32H41X_USART4) || \
    defined(CONFIG_CH32H41X_USART5) || defined(CONFIG_CH32H41X_USART6) || \
    defined(CONFIG_CH32H41X_USART7) || defined(CONFIG_CH32H41X_USART8)
#  define HAVE_UART_DEVICE 1
#endif

#endif /* __ARCH_RISCV_SRC_CH32H41X_CH32H41X_CONFIG_H */
