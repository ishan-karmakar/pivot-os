#pragma once
#include <stddef.h>

#define NO_SYS 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0

#define MEM_ALIGNMENT 8

// For some reason it only works when defined in this file, not sys_arch.h
typedef char sys_prot_t;
