#!/bin/sh
# tools/sync-dirlinks-msys.sh
#
# SPDX-License-Identifier: Apache-2.0
#
# With CONFIG_WINDOWS_MSYS the NuttX dirlink mechanism creates .fakelnk
# marked copies (tools/copydir.sh).  The make rules that create them treat
# an existing destination directory as "up to date", so after editing
# chip/board sources the copies must be deleted and re-created.  To keep
# incremental builds fast the copies are only replaced when their content
# actually differs:
#
#   sh tools/sync-dirlinks-msys.sh          # refresh copies if stale
#   sh tools/sync-dirlinks-msys.sh clean    # remove them and distclean
#
# Usage: sh tools/sync-dirlinks-msys.sh [clean]

set -e
cd "$(dirname "$0")/.."

refresh()
{
  src="$1"
  dst="$2"
  if [ -d "$dst" ] && diff -qr -x ".fakelnk" "$src" "$dst" > /dev/null 2>&1; then
    return
  fi
  if [ -e "$dst" ]; then
    rm -rf "$dst"
  fi
  mkdir -p "$(dirname "$dst")"
  cp -r "$src" "$dst"
  # bump the copied file timestamps so make sees the change, and mark the
  # copy as a fake link so unlink.sh accepts it at distclean
  find "$dst" -type f -exec touch {} +
  touch "$dst/.fakelnk"
  echo "sync: refreshed $dst"
}

if [ "X$1" = "Xclean" ]; then
  for d in include/arch/chip include/arch/board arch/risc-v/src/board arch/risc-v/src/chip include/arch; do
    if [ -e "$d" ]; then
      rm -rf "$d"
      echo "clean: $d"
    fi
  done
  rm -f .dirlinks ../NuttX-apps/.dirlinks 2>/dev/null || true
  make distclean ${MAKE_OPTS}
  exit 0
fi

for pair in \
  "arch/risc-v/include/ch32h41x include/arch/chip" \
  "boards/risc-v/ch32h41x/ch32h41x-evb/include include/arch/board" \
  "arch/risc-v/src/ch32h41x arch/risc-v/src/chip" \
  "boards/risc-v/ch32h41x/ch32h41x-evb/src arch/risc-v/src/board"; do
  refresh $(eval echo $pair)
done

echo "sync: ok"
