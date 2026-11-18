/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_head.S
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

	.section .init, "ax", @progbits

#include "riscv_internal.h"

	.globl	_start
	.globl	__start
	.type	_start, @function
_start:
__start:
	/* Force the PC to the linked entry address.  The V5F is woken by
	 * the V3F through PFIC.WAKEIP[1] with an address whose low 10 bits
	 * are masked off, so the reset fetch may not land exactly here;
	 * an absolute set of ra + jump keeps the boot correct regardless
	 * of where the core started fetching (BF startup_ch32h417_v5f.S
	 * does the same). */

	lui		ra, %Hi(handle_reset)
	jr		%Lo(handle_reset)(ra)
	.size	_start, . - _start

	.section	.flashcode,"ax",@progbits
	.align	2
	.type	handle_reset, @function
handle_reset:
.option push
.option	norelax
	la		gp, __global_pointer$
.option	pop

	/* Boot stack: idle-task stack inside DTCM, heap follows it */

	la		sp, _ebss
	li		t0, SMP_STACK_SIZE
	add		sp, sp, t0

	.macro COPYSECTION lma, vma, end
	la		a0, \lma
	la		a1, \vma
	la		a2, \end
	bgeu	a1, a2, 99f
1:
	lw		t0, 0(a0)
	sw		t0, 0(a1)
	addi	a0, a0, 4
	addi	a1, a1, 4
	bltu	a1, a2, 1b
99:
	.endm

	/* Vectors + fast code -> ITCM (0x200A0000, 256KiB) */

	COPYSECTION _vector_lma, _vector_vma, _vector_end
	COPYSECTION _tcm_code_lma, _tcm_code_vma, _tcm_code_end

	/* The rest of the code and constants -> SRAM (0x20105000, 492KiB) */

	COPYSECTION _ram_code_lma, _ram_code_vma, _ram_code_end

	/* init/fini arrays and runtime data -> DTCM (0x200E0200, ~128KiB) */

	COPYSECTION _initarray_lma, _initarray_vma, _initarray_end
	COPYSECTION _finiarray_lma, _finiarray_vma, _finiarray_end
	COPYSECTION _data_lma, _data_vma, _data_end

	/* Clear the DTCM bss */

	la		a0, _sbss
	la		a1, _ebss
	bgeu	a0, a1, 2f
1:
	sw		zero, 0(a0)
	addi	a0, a0, 4
	bltu	a0, a1, 1b
2:

	/* Make the ITCM/SRAM/DTCM stores globally visible before any of
	 * the copied code is fetched (V5F is out-of-order dual-issue). */

	fence.i

	/* QingKe core configuration - the same CSR sequence as the proven
	 * ch32_px4_firmware_nuttx-12.12.0+ startup (its CONFIG_CH32H41X_
	 * CORE_V5F variant): 0xBC0 core feature config, INTSYCR = 0 and
	 * nesting level 0.  mstatus is left untouched: the FPU state
	 * (mstatus.FS) is enabled by riscv_fpuconfig() in ch32h41x_start,
	 * before any FP code can run. */

	li		t0, 0x1237b3e0
	csrw	0xbc0, t0

	li		t0, 0
	csrw	0x804, t0

	li		t0, 0
	csrw	0xbc1, t0

	la		t0, _vector_base
	ori		t0, t0, 3
	csrw	mtvec, t0

	call	ch32h41x_start

1:
	j		1b
	.size	handle_reset, . - handle_reset
