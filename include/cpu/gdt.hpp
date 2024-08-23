#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>

namespace gdt {
    constexpr uint16_t KCODE = 0x8;
    constexpr uint16_t KDATA = 0x10;
    constexpr uint16_t UCODE =  0x18;
    constexpr uint16_t UDATA = 0x20;

    union desc {
        struct [[gnu::packed]] {
            uint16_t limit0;
            uint16_t base0;
            uint8_t base1;
            uint8_t access_byte;
            uint8_t limit1:4;
            uint8_t flags:4;
            uint8_t base2;
        };
        uint64_t raw;
    };

    extern uint16_t num_entries;

    void early_init();
    void init();
    void set(uint16_t, uint8_t, uint8_t);
    void set(uint16_t, desc);
    desc get(uint16_t);
    void load();
}

