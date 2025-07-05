#pragma once
#include <stddef.h>
#include <stdint.h>

#define LWIP_SOCKET 0
#define LWIP_NETCONN 0

#define MEM_CUSTOM_ALLOCATOR 1
#define MEM_CUSTOM_MALLOC lwip_malloc
#define MEM_CUSTOM_CALLOC lwip_calloc
#define MEM_CUSTOM_FREE lwip_free
#define MEMP_MEM_MALLOC 1

typedef uint8_t sys_prot_t;

void *lwip_malloc(size_t);
void lwip_free(void *);
void *lwip_calloc(size_t, size_t);