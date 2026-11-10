/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/tools-host/sys/utsname.h
 *
 * Stub <sys/utsname.h> for building the NuttX host tools with zig cc on
 * Windows (the mingw-w64 headers shipped with zig do not provide it).
 * The tools only use uname() for output formatting; returning failure
 * selects the portable Unix behaviour which is what the GNU toolchain
 * expects.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.
 */

#ifndef __STUB_SYS_UTSNAME_H
#define __STUB_SYS_UTSNAME_H

struct utsname
{
  char sysname[32];
  char nodename[32];
  char release[32];
  char version[32];
  char machine[32];
  char domainname[32];
};

static inline int uname(struct utsname *buf)
{
  if (buf == 0)
    {
      return -1;
    }

  /* Report a POSIX-like system so the host tools emit GNU toolchain
   * style output (plain "-I" flags, forward slashes).
   */

  __builtin_strncpy(buf->sysname, "Linux", sizeof(buf->sysname));
  __builtin_strncpy(buf->nodename, "host", sizeof(buf->nodename));
  __builtin_strncpy(buf->release, "1", sizeof(buf->release));
  __builtin_strncpy(buf->version, "1", sizeof(buf->version));
  __builtin_strncpy(buf->machine, "x86_64", sizeof(buf->machine));
  __builtin_strncpy(buf->domainname, "", sizeof(buf->domainname));
  return 0;
}

#endif /* __STUB_SYS_UTSNAME_H */
