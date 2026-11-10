NuttX on the WCH CH32H41x Evaluation Board (CH32H417 EVT)
==========================================================

Board Overview
--------------

  The WCH CH32H41x evaluation board carries a CH32H417QEU with dual
  QingKe RV32IMAFCB-X cores (V3F + V5F), 960KiB zero-wait code flash,
  512KiB shared SRAM, and a 25MHz HSE crystal.  NuttX runs on the V3F
  boot master core:

  - System clock: 480MHz (USBHS PLL from the 25MHz HSE)
  - V5F core: 240MHz
  - HCLK = V3F core = peripherals: 120MHz
  - Console: USART1 on PA9 (TX) / PA10 (RX), AF7, 115200 8N1

  Code is stored in the zero-wait flash but executes from SRAM: the
  linker script places each flash-resident section with a flash load
  address and an SRAM virtual address; ch32h41x_head.S copies them one
  by one before starting the scheduler.

  Interrupts: the WCH PFIC is used in the absolute-address vector table
  mode (mtvec[1:0] = 3) with a 1KiB aligned table whose entries all
  point to the common NuttX exception entry.  This mode is the only one
  supported by the V5F core, so the interrupt architecture is shared by
  both cores (the V5F bring-up is planned for a later stage).

Build environment (Git Bash on Windows)
---------------------------------------

  Reference toolchain: the WCH GCC 12.2.0 shipped with MounRiver Studio 2
  ("RISC-V Embedded GCC12").  The host also needs:

  - GNU make that understands POSIX paths (an MSYS2 make; the mingw make
    bundled with MounRiver cannot parse "F:/..." in target names).  A
    known-good copy is expected at C:\Users\tempe\.nuttx-hostbin\make.
  - kconfiglib (pip install kconfiglib) plus the kconfig-tweak shim and
    the olddefconfig wrapper installed next to it - these translate the
    generated apps/Kconfig absolute paths for the Windows python.
  - zig (pip install ziglang) to build the host tools (mkconfig, mkdeps,
    incdir) - the scripts/Make.defs of this board sets HOSTCC
    accordingly.

  Environment (Git Bash):

    export PATH="/c/Users/tempe/.nuttx-hostbin:$PATH"
    export PATH="$PATH:/c/Users/tempe/AppData/Roaming/Python/Python315/Scripts"
    export PATH="$PATH:/d/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin"

Configuring and building
------------------------

  The recommended way is the one-command script, which sets up the PATH,
  configures with the required MinGW/MSYS host flags and runs make:

    sh tools/build-ch32h41x-evb.sh            # build
    sh tools/build-ch32h41x-evb.sh distclean  # clean

  Manual equivalent:

  1. ./tools/configure.sh ch32h41x-evb:nsh -a ../NuttX-apps -g

     The -g option (Windows host, MSYS environment) is REQUIRED on this
     machine.  configure.sh lower-cases the apps directory name; restore
     it:

  2. sed -i 's|nuttx-apps|NuttX-apps|' .config

  3. make

     Result: nuttx/nuttx.hex/nuttx.bin.  The code is compiled for
     rv32imac with the ilp32 (soft-float) ABI.

  NOTE on build output: on a fresh build the dependency pre-scan prints
  hundreds of "fatal error: nuttx/config.h: No such file or directory"
  lines.  This is noise from the mkdeps pass running before the
  generated headers exist; the real compile pass succeeds afterwards.
  Trust the exit code of make and the presence of nuttx.hex, not the
  error count.  If configure.sh is ever re-run without -g, restore the
  host setup with "tools/sethost.sh -g" (the build script does this
  automatically).

  Cleaning: make distclean works normally with the fake-link (.fakelnk)
  copies that the MSYS dirlink mechanism produces.

  After editing chip/board sources, force the .dirlinks to be redone:

    rm -f .dirlinks ../NuttX-apps/.dirlinks && make

Loading and running
-------------------

  Flash the generated nuttx.hex at address 0x00000000 (the code flash
  window; the programming alias is 0x08000000):

  - MounRiver Studio: create a project pointing at nuttx.elf, select
    the WCH-Link probe, "Flash Program".
  - openocd (wch-link): flash 0 nuttx.hex, then reset.

  Open a serial terminal on the board's USB serial port at 115200 8N1;
  the nsh> prompt should appear shortly after reset.  Verify with:

    uname -a
    uptime

Known limitations
-----------------

  - No FPU support yet (soft-float only); the V3F/V5F single-precision
    FPU requires context-switch support to be added first.
  - Only USART and GPIO drivers are present at this stage.
  - The V5F configuration exists as a placeholder only.
  - The NuttX-apps tree is the PX4 fork of the 10.3 era; a few
    compatibility patches were applied (see the port notes) and a full
    apps upgrade to the 12.x APIs is still pending.
