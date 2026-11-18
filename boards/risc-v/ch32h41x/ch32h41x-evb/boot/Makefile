# boards/risc-v/ch32h41x/ch32h41x-evb/boot/Makefile
#
# V3F boot loader (dual-core split): tiny bare-metal image at flash
# 0x00000000 that brings the clocks up and wakes the V5F NuttX image.
# Standalone build - invoked directly or from tools/build-ch32h41x-evb.sh.

TOOLCHAIN := "/d/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin"
CC      := $(TOOLCHAIN)/riscv-wch-elf-gcc
OBJCOPY := $(TOOLCHAIN)/riscv-wch-elf-objcopy
SIZE    := $(TOOLCHAIN)/riscv-wch-elf-size

CFLAGS  := -march=rv32imafdc_zicsr_zifencei -mabi=ilp32f -mcmodel=medany \
           -O2 -g -Wall -ffreestanding -nostdlib \
           -I../include -IF:/1.FC_ESC/27.PX4/NuttX/arch/risc-v/src/ch32h41x

all: boot.bin

OBJS    := boot_head.o boot_clock.o boot_main.o

boot.elf: $(OBJS) boot.ld
	$(CC) $(CFLAGS) -T boot.ld -nostartfiles -Wl,-Map=boot.map -o $@ $(OBJS)
	$(SIZE) $@

boot.bin: boot.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o boot.elf boot.bin boot.map

