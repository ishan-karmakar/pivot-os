#include <cstddef>
#include <cstdint>
#include <misc/cxxabi.hpp>
#include <util/logger.hpp>
#include <cstdlib>
#include <cxxabi.h>
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
    namespace __cxxabiv1 {
        int __cxa_guard_acquire(__guard *guard) {
            if (*guard & 1)
                return 0;
            else if (*guard & 0x100)
                abort();
            
            *guard |= 0x100;
            return 1;
        }

        void __cxa_guard_release(__guard *guard) {
            *guard |= 1;
        }
    }
}
