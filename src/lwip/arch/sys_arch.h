#pragma once
#include <stdbool.h>

typedef int sys_sem_t;

// Make sure that sizeof(sys_mutex_t) == sizeof(std.atomic.Value(bool))
typedef bool sys_mutex_t;

// Actual structure is defined inside Zig code
typedef void* sys_mbox_t;

// Actual structure is defined inside Zig code
typedef void* sys_thread_t;

#define ENOMEM 12
#define ENOBUFS 105
#define EWOULDBLOCK 11
#define EHOSTUNREACH 113
#define EINPROGRESS 115
#define EINVAL 22
#define EADDRINUSE 98
#define EALREADY 114
#define EISCONN 106
#define ENOTCONN 107
#define ECONNABORTED 103
#define ECONNRESET 104
#define ENOTCONN 107
#define EIO 5