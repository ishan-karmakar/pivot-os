#pragma once
#include <cstdint>
#include <cpu/idt.hpp>
#define PIT_IDT_ENT 34

namespace io {
    class PIT {
    public:
        PIT() = delete;
        ~PIT() = delete;

        static void init(cpu::IDT&);
        static void cmd(bool, uint8_t, uint8_t, uint8_t);

        static uint16_t ticks;
    };
}