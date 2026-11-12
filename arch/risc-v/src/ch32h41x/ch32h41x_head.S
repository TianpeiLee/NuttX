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
	j	handle_reset
	.size	_start, . - _start

	.section	.flashcode,"ax",@progbits
	.align	2
	.type	handle_reset, @function
handle_reset:
.option push
.option	norelax
	la		gp, __global_pointer$
.option	pop

	/* Only the boot master (hart 0, V3F) proceeds with the boot
	 * sequence.  The V5F (hart 1) parks here to avoid racing the
	 * V3F through the copy/zero/init path, which would corrupt
	 * the SRAM contents that both cores share. */

	csrr	a5, mhartid
	bnez	a5, 99f

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

	COPYSECTION _vector_lma, _vector_vma, _vector_end
	COPYSECTION _text_lma, _text_vma, _text_end
	COPYSECTION _rodata_lma, _rodata_vma, _rodata_end
	COPYSECTION _initarray_lma, _initarray_vma, _initarray_end
	COPYSECTION _finiarray_lma, _finiarray_vma, _finiarray_end
	COPYSECTION _data_lma, _data_vma, _data_end

	la		a0, _sbss
	la		a1, _ebss
	bgeu	a0, a1, 2f
1:
	sw		zero, 0(a0)
	addi	a0, a0, 4
	bltu	a0, a1, 1b
2:

#ifdef CONFIG_CH32H41X_CORE_V5F
	li		t0, 0x1237b3e0
#else
	li		t0, 0x123703e1
#endif
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

	/* Park: V5F core spins here forever waiting for a (never-coming)
	 * interrupt.  It can be released later by SMP support or a debugger
	 * that sets its PC to a valid entry point. */

99:
	wfi
	j		99b
	.size	handle_reset, . - handle_reset
