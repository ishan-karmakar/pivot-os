#pragma once
#include <cpu/idt.hpp>
#include <cpu/cpu.hpp>
#define RTC_IDT_ENT 34

namespace cpu {
    extern "C" cpu::cpu_status *rtc_handler(cpu::cpu_status*);
}

namespace drivers {
    class RTC {
    public:
        // I'm using this weird union for storing time so that bcd2bin doesn't have to be repeated every time
        union time {
            struct {
                uint8_t century;
                uint8_t year;
                uint8_t month;
                uint8_t dom;
                uint8_t hour;
                uint8_t minute;
                uint8_t second;
                uint8_t dow;
            };
            uint8_t data[8];
        };

        static void init(cpu::IDT&);
        static time time;
    
    private:
        static uint8_t read_reg(uint8_t);
        static void write_reg(uint8_t, uint8_t);
        static void get_time();
        static uint8_t bcd2bin(uint8_t);
        static uint8_t get_dow(uint8_t, uint8_t, uint8_t);
        static bool bcd;

        friend cpu::cpu_status *cpu::rtc_handler(cpu::cpu_status*);
    };
}