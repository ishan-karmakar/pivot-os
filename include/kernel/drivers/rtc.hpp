#pragma once
#include <cpu/idt.hpp>
#define RTC_IDT_ENT 34

namespace drivers {
    class RTC {
    public:
        static void init(cpu::IDT&);
    
    private:
        static uint8_t read_reg(uint8_t);
        static void write_reg(uint8_t, uint8_t);

        static bool bcd;
    };
}
