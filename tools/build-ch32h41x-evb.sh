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
#   sh tools/build-ch32h41x-evb.sh              # incremental build
#   sh tools/build-ch32h41x-evb.sh distclean    # full clean
#   sh tools/build-ch32h41x-evb.sh reconfig     # distclean + reconfigure + build
#
# NOTE: always use this script (or tools/configure.sh with -g).  Running
# tools/configure.sh without the -g option selects the Linux host setup,
# which cannot build on this machine.
#
# ---------------------------------------------------------------------------
# Bootstrap invariants on MSYS (why this script looks the way it does):
#
# 1. The dirlink mechanism on this host makes .fakelnk-marked COPIES
#    (tools/copydir.sh).  Five chip/board copy pairs are refreshed by
#    tools/sync-dirlinks-msys.sh; two more copies that distclean removes
#    and nothing else restores are handled there as well:
#      drivers/platform                 <- drivers/dummy
#      NuttX-apps/platform/board        <- NuttX-apps/platform/dummy
#
# 2. `make olddefconfig` internally runs `clean_context`, which deletes
#    those copies.  Therefore the sync script must run AFTER any
#    olddefconfig/configure step and BEFORE compiling.
#
# 3. A standalone kconfig olddefconfig (no make environment) aborts on
#    `source "$BINDIR/..."` (arch/Kconfig) and then writes NO .config at
#    all, losing the derived settings (CONFIG_ARCH="risc-v" &c.).  The
#    .config must only ever be generated through `make olddefconfig` /
#    tools/configure.sh, which set up the KCONFIG_ENV.
# ---------------------------------------------------------------------------

set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Host tools: msys2 make, kconfiglib + kconfig-tweak shim + olddefconfig
# wrapper, zig (host compiler for mkconfig/mkdeps/incdir), WCH toolchain.

NUTTX_HOSTBIN="/c/Users/tempe/.nuttx-hostbin"
KCONFIGLIB_BIN="/c/Users/tempe/AppData/Roaming/Python/Python315/Scripts"
WCH_GCC="/d/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC12/bin"
export PATH="$NUTTX_HOSTBIN:$KCONFIGLIB_BIN:$WCH_GCC:$PATH"

configure()
{
  # Configure on first use.  -g selects the Windows/MSYS host setup, which
  # is required on this machine.  NOTE: configure.sh stops parsing options
  # at the board:config argument, so all options must come BEFORE it.

  ./tools/configure.sh -a ../NuttX-apps -g ch32h41x-evb:nsh
  # configure.sh lower-cases the apps directory name; restore it
  sed -i 's|nuttx-apps|NuttX-apps|' .config

  # Re-assert the host selection if a manual configure without -g lost it.

  if ! grep -q "^CONFIG_WINDOWS_MSYS=y" .config; then
    echo "build: restoring Windows/MSYS host setup (CONFIG_WINDOWS_MSYS)"
    ./tools/sethost.sh -g
  fi
}

case "$1" in
  distclean)
    # Removes the .fakelnk copies and the whole build state.  The next
    # plain invocation reconfigures from boards/.../defconfig.
    sh tools/sync-dirlinks-msys.sh clean
    exit 0
    ;;

  reconfig)
    # Full rebuild from the defconfig: clean, configure, build.
    sh tools/sync-dirlinks-msys.sh clean
    configure
    sh tools/sync-dirlinks-msys.sh
    make context
    make -j4 all
    exit $?
    ;;
esac

# Incremental build path ------------------------------------------------------------

# Restore the two extra copies that distclean/clean_context remove and
# that no make rule recreates.  Run before everything else.

sh tools/sync-dirlinks-msys.sh

if [ ! -f .config ]; then
  configure
  # configure.sh ran make olddefconfig internally; its clean_context step
  # deleted the copies again, so refresh once more before compiling.
  sh tools/sync-dirlinks-msys.sh
else
  # Keep the .config in sync with the defconfig for incremental builds:
  # run olddefconfig through make (which exports BINDIR &c. for arch/Kconfig),
  # then restore the copies that its clean_context step deleted.
  make olddefconfig
  sh tools/sync-dirlinks-msys.sh
fi

# Run context separately: regenerating include/nuttx/config.h inline with a
# parallel make races against the first compile waves on MSYS.

make context
make -j4 "$@"   # -j8 races the apps archive steps on MSYS (members lost)
