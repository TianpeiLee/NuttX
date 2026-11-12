/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_allocateheap.c
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

#include <stdint.h>
#include <stddef.h>

#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>

#include "riscv_internal.h"
#include "chip.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   The heap extends from the top of the idle task stack to the end of
 *   the core-local RAM region (__heap_end is provided by the board linker
 *   script).
 *
 ****************************************************************************/

extern uint8_t __heap_end[];

void up_allocate_heap(void **heap_start, size_t *heap_size)
{
  /* Reserve a guard zone (256 bytes) between the top of the idle task
   * stack and the heap.  The FPU context save buffer for the IDLE task
   * is allocated at the very start of the heap and the floating-point
   * save/restore routines have been observed writing a few bytes past
   * their area; without this reservation they corrupt the heap guard
   * node and the first allocations overlap each other. */

  *heap_start = (void *)(g_idle_topstack + 256);
  *heap_size  = (size_t)((uintptr_t)__heap_end - g_idle_topstack - 256);
}
