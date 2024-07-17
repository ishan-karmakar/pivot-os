#include <util/logger.h>
#include <cstddef>
#include <cstdint>
typedef void (*func_t)(void);

extern uintptr_t __start_ctors;
extern uintptr_t __stop_ctors;

extern char __start_dtors;
extern char __stop_dtors;

void call_constructors() {
    size_t num_entries = &__stop_ctors - &__start_ctors;
    uintptr_t *init_array = &__start_ctors;
    for (size_t i = 0; i < num_entries; i++) {
        ((void (*)()) init_array[i])();
    }
}