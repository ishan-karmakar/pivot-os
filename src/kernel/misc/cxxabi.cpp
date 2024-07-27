#include <cstddef>
#include <cstdint>
#include <misc/cxxabi.hpp>
#include <util/logger.h>
#include <cstdlib>
typedef void (*func_t)(void);

extern uintptr_t __start_ctors;
extern uintptr_t __stop_ctors;

extern char __start_dtors;
extern char __stop_dtors;

void cxxabi::call_constructors() {
    size_t num_entries = &__stop_ctors - &__start_ctors;
    uintptr_t *init_array = &__start_ctors;
    for (size_t i = 0; i < num_entries; i++)
        ((func_t) init_array[i])();
    log(Info, "CXXABI", "Called global constructors");
}

extern "C" {
    [[noreturn]]
    void __stack_chk_fail() {
        log(Error, "KERNEL", "Detected stack smashing");
        abort();
    }

    // [[noreturn]]
    // void __cxa_pure_virtual() {
    //     log(Error, "KERNEL", "Could not find virtual function");
    //     abort();
    // }

    int __cxa_atexit(void*, void*, void*) {
        return 0;
    }
}