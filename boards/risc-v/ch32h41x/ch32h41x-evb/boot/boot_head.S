/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/boot/boot_head.S
 *
 * V3F boot loader entry.  Cold power-on starts the V3F here (PC = 0).
 * This image only: sets the stack, copies .data, clears .bss, then
 * calls boot_main() which brings the clocks up and wakes the V5F.
 *
 * IMAGE HEAD STRUCTURE (matches every working V3F image - EVT demos,
 * the official NuttX port, the proven ch32_px4_firmware branch):
 *   0x0: j handle_reset
 *   0x4: vector table (word0 = _start, then handler words).  The WCH
 *        bootROM handoff expects the table at the head of the image.
 ****************************************************************************/

	.section .init, "ax", @progbits
	.globl	_start
	.type	_start, @function
_start:
	j	handle_reset
	.size	_start, . - _start

	.section .vectors, "ax", @progbits
	.globl	_boot_vectors
	.align	1
_boot_vectors:
	.option	norvc
	.word	_start
	.word	0
	.word	boot_trap
	.word	boot_trap
	.word	0
	.word	boot_trap
	.word	0
	.word	0
	.word	boot_trap
	.word	boot_trap
	.word	0
	.word	0
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	0
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	.word	boot_trap
	/* External interrupt slots: the loader enables no interrupts; all
	 * slots get the same landing pad so nothing can run wild. */

	.rept	200
	.word	boot_trap
	.endr
	.option	rvc

	.section .text
	.align	1
	.globl	boot_trap
boot_trap:
	wfi
	j	boot_trap

	.section .flashcode,"ax",@progbits
	.align	2
	.type	handle_reset, @function
handle_reset:
.option push
.option	norelax
	la	gp, __global_pointer$
.option	pop

	/* The WCH bootROM lets BOTH cores enter at 0x00000000.  Only the
	 * boot master (hart 0, V3F) runs the loader; the V5F parks here
	 * and is redirected to 0x00010000 by the WAKEIP write at the end
	 * of boot_main. */

	csrr	a5, mhartid
	bnez	a5, .Lpark_v5f

	/* Stack at the top of the 20KiB V3F SRAM window */

	li	sp, 0x20105000

	/* Copy .data from flash to SRAM */

	la	a0, _data_lma
	la	a1, _data_vma
	la	a2, _data_end
	bgeu	a1, a2, 2f
1:
	lw	t0, 0(a0)
	sw	t0, 0(a1)
	addi	a0, a0, 4
	addi	a1, a1, 4
	bltu	a1, a2, 1b
2:

	/* Clear .bss */

	la	a0, _sbss
	la	a1, _ebss
	bgeu	a0, a1, 2f
1:
	sw	zero, 0(a0)
	addi	a0, a0, 4
	bltu	a0, a1, 1b
2:

	/* No interrupts are used by the loader: direct vector mode, one
	 * landing pad for everything that could ever fire */

	la	t0, boot_trap
	csrw	mtvec, t0

	call	boot_main

.Lpark_v5f:
1:
	wfi
	j	1b
	.size	handle_reset, . - handle_reset
