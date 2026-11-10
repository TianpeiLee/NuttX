#!/bin/sh
# tools/build-ch32h41x-evb.sh
#
# SPDX-License-Identifier: Apache-2.0
#
# One-command build for the ch32h41x-evb:nsh configuration on this
# Windows (Git Bash) host.  It sets up the host tool PATH (MSYS make,
# kconfiglib shims, zig host compiler, WCH cross toolchain), configures
# with the required MinGW/MSYS host flags and runs the build.  Extra
# arguments are passed to make, e.g.:
#
#   sh tools/build-ch32h41x-evb.sh              # build
#   sh tools/build-ch32h41x-evb.sh distclean    # clean
#
# NOTE: always use this script (or tools/configure.sh with -g).  Running
# tools/configure.sh without the -g option selects the Linux host setup,
# which cannot build on this machine.

set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Host tools: msys2 make, kconfiglib + kconfig-tweak shim + olddefconfig
# wrapper, zig (host compiler for mkconfig/mkdeps/incdir), WCH toolchain.

NUTTX_HOSTBIN="/c/Users/tempe/.nuttx-hostbin"
KCONFIGLIB_BIN="/c/Users/tempe/AppData/Roaming/Python/Python315/Scripts"
WCH_GCC="/d/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin"
export PATH="$NUTTX_HOSTBIN:$KCONFIGLIB_BIN:$WCH_GCC:$PATH"

# Configure on first use.  -g selects the Windows/MSYS host setup, which
# is required on this machine.  NOTE: configure.sh stops parsing options
# at the board:config argument, so all options must come BEFORE it.

if [ ! -f .config ]; then
  ./tools/configure.sh -a ../NuttX-apps -g ch32h41x-evb:nsh
  # configure.sh lower-cases the apps directory name; restore it
  sed -i 's|nuttx-apps|NuttX-apps|' .config
fi

# Re-assert the host selection if a manual configure without -g lost it.

if ! grep -q "^CONFIG_WINDOWS_MSYS=y" .config; then
  echo "build: restoring Windows/MSYS host setup (CONFIG_WINDOWS_MSYS)"
  ./tools/sethost.sh -g
fi

# Refresh the .fakelnk copies of the chip/board sources (they are only
# re-created when the .dirlinks stamps are missing)

sh tools/sync-dirlinks-msys.sh

# Run context separately: regenerating include/nuttx/config.h inline with a
# parallel make races against the first compile waves on MSYS.

make context
make -j8 "$@"
