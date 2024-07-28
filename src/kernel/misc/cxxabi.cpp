#include <cstddef>
#include <cstdint>
#include <misc/cxxabi.hpp>
#include <util/logger.hpp>
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
    int __popcountdi2(long long a) {
        unsigned long long x2 = (unsigned long long)a;
        x2 = x2 - ((x2 >> 1) & 0x5555555555555555uLL);
        // Every 2 bits holds the sum of every pair of bits (32)
        x2 = ((x2 >> 2) & 0x3333333333333333uLL) + (x2 & 0x3333333333333333uLL);
        // Every 4 bits holds the sum of every 4-set of bits (3 significant bits) (16)
        x2 = (x2 + (x2 >> 4)) & 0x0F0F0F0F0F0F0F0FuLL;
        // Every 8 bits holds the sum of every 8-set of bits (4 significant bits) (8)
        unsigned x = (unsigned)(x2 + (x2 >> 32));
        // The lower 32 bits hold four 16 bit sums (5 significant bits).
        //   Upper 32 bits are garbage
        x = x + (x >> 16);
        // The lower 16 bits hold two 32 bit sums (6 significant bits).
        //   Upper 16 bits are garbage
        return (x + (x >> 8)) & 0x0000007F; // (7 significant bits)
    }
}
