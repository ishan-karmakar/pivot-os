#pragma once

#define NO_SYS 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_IPV6 0
#define LWIP_TIMERS 0

#define MEMCPY(dst, src, len)
#define SMEMCPY(dst, src, len)
#define SYS_ARCH_PROTECT(lev)
#define SYS_ARCH_DECL_PROTECT(lev)
#define SYS_ARCH_UNPROTECT(lev)

typedef int sys_prot_t;
