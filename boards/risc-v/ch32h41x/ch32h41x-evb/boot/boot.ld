/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/boot/boot.ld
 *
 * V3F boot loader memory layout (dual-core split):
 *   FLASH 0x00000000, 64KiB  - this image (the V5F NuttX image starts at
 *                              0x08010000 / 0x00010000)
 *   SRAM  0x20100000, 20KiB  - data/bss/stack (the NuttX SRAM code region
 *                              starts at 0x20105000)
 */

OUTPUT_ARCH("riscv")

ENTRY(_start)

MEMORY
{
  flash (rx) : ORIGIN = 0x00000000, LENGTH = 64K
  sram (rwx) : ORIGIN = 0x20100000, LENGTH = 20K
}

SECTIONS
{
  .init :
  {
    KEEP(*(SORT_NONE(.init)))
    KEEP(*(.vectors))
    . = ALIGN(4);
  } > flash

  .flashcode :
  {
    . = ALIGN(4);
    KEEP(*(SORT_NONE(.flashcode)))
    . = ALIGN(4);
  } > flash

  .text :
  {
    . = ALIGN(4);
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
    _etext = .;
  } > flash

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data .data.*)
    . = ALIGN(8);
    PROVIDE(__global_pointer$ = . + 0x800);
    *(.sdata .sdata.*)
    . = ALIGN(4);
    _edata = .;
  } > sram AT> flash

  _data_lma = LOADADDR(.data);
  _data_vma = ADDR(.data);
  _data_end = ADDR(.data) + SIZEOF(.data);

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > sram

  /* Boot stack at the top of the V3F SRAM window */

  _eusrstack = ORIGIN(sram) + LENGTH(sram);

  /DISCARD/ :
  {
    *(.eh_frame)
  }
}
