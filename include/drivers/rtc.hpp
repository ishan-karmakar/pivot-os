#pragma once
namespace cpu {
    extern "C" cpu::status *rtc_handler(cpu::status*);
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

        static void init();
        static time time;
    
    private:
        static uint8_t read_reg(uint8_t);
        static void write_reg(uint8_t, uint8_t);
        static void fetch_time();
        static uint8_t bcd2bin(uint8_t);
        static void set_dow();

        friend cpu::status *cpu::rtc_handler(cpu::status*);

        static bool bcd;
        static constexpr int IDT_ENT = 34;
        static constexpr int IRQ_ENT = 8;
    };
}