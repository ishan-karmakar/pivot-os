#pragma once
#include <stdint.h>
#define ERR_MASK ((uint64_t) 1 << 63)
#define ERR(status) (status | ERR_MASK)
#define EFI_ERR(status) (status & ERR_MASK)

typedef uint64_t efi_status_t;
