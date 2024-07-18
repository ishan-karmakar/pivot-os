#pragma once
#include <cstdint>
#include <cpu/idt.hpp>
#define PIT_IDT_ENT 34
#define PIT_MS 1193

namespace drivers {
    class PIT {
    public:
        PIT() = delete;
        ~PIT() = delete;

        static void init(cpu::IDT&);
        static void cmd(bool, uint8_t, uint8_t, uint8_t);
        static void data(uint16_t);

        static volatile size_t ticks;
    };
}