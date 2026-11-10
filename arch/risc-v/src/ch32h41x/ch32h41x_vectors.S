/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_vectors.S
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

/* WCH QingKe cores only support the vectored mtvec modes; the V5F has no
 * flat (single entry) mode at all.  We therefore use the absolute-address
 * vector mode (mtvec[1:0] = 3) on both cores.  In this mode the core looks
 * up the trap entry in the table by the mcause code, so every slot points
 * to the NuttX common entry exception_common, which passes the raw mcause
 * to riscv_dispatch_irq() for software routing.  The PFIC interrupt enable
 * state and the NuttX irq numbering stay identical across the V3F and V5F.
 *
 * The table must be 1KiB aligned (WCH requirement); enforced by the
 * linker script which places the .vector section on a 0x400 boundary in
 * SRAM.  The head.S startup copies the table to SRAM before pointing
 * mtvec at it.
 *
 * Vector indices: 0..15 exception codes, 16..31 core interrupts,
 * 32..148 peripheral interrupts (see arch/risc-v/include/ch32h41x/irq.h).
 */

  .section .vector, "ax", @progbits
  .option norvc

  .globl _vector_base
  .type  _vector_base, @object

_vector_base:
  .rept 149
  .word exception_common
  .endr

  .size _vector_base, . - _vector_base

  .option rvc
