/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/src/ch32h41xevb_boot.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>

#include "ch32h41x_gpio.h"
#include "ch32h41xevb.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_boardinitialize
 *
 * Description:
 *   Perform chip-level (not application-level) board initialization.
 *   Called by ch32h41x_start() early during boot.
 *
 ****************************************************************************/

void ch32h41x_boardinitialize(void)
{
  /* Enable the AFIO and GPIO port clocks; the console pins were already
   * configured by ch32h41x_lowsetup().
   */

  ch32h41x_gpioinit();
}
