#pragma once
#include <cstddef>
#include <cstdint>

namespace cpu {
    struct status {
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t r11;
        uint64_t r10;
        uint64_t r9;
        uint64_t r8;
        uint64_t rdi;
        uint64_t rsi;
        uint64_t rbp;
        uint64_t rdx;
        uint64_t rcx;
        uint64_t rbx;
        uint64_t rax;

        uint64_t int_no;
        uint64_t err_code;

        uint64_t rip;
        uint64_t cs;
        uint64_t rflags;
        uint64_t rsp;
        uint64_t ss;
    };

    [[noreturn]]
    inline void hcf() {
        asm volatile ("cli; hlt");
        __builtin_unreachable();
    }

    inline uint64_t rdmsr(uint32_t address) {
        uint32_t low = 0, high = 0;
        asm volatile (
            "rdmsr"
            : "=a" (low), "=d" (high)
            : "c" (address)
        );
        return (uint64_t) low | ((uint64_t) high << 32);
    }

    inline void wrmsr(uint32_t address, uint64_t value) {
        asm volatile ("wrmsr" : : "a" ((uint32_t) value), "d" (value >> 32), "c" (address));
    }

    inline void set_kgs(uint64_t a) {
        wrmsr(0xC0000102, a);
    }

    inline void set_gs(uint64_t a) {
        wrmsr(0xC0000101, a);
    }

    inline uint64_t get_kgs() {
        return rdmsr(0xC0000102);
    }

    inline uint64_t get_gs() {
        return rdmsr(0xC0000101);
    }

    [[gnu::noinline]]
    uintptr_t rip();

    void init();
}

// __attribute__((always_inline))
// static inline void load_cr3(uintptr_t addr) {
//     asm volatile ("mov %0, %%cr3" :: "r" (addr) : "memory");
// }

// void syscall(size_t, size_t, ...);