#pragma once
#include <cstdint>

namespace drivers {
    class PS2 {
    public:
        PS2() = delete;
        static void init();
        static uint8_t get_config();
        static void set_config(uint8_t);
        static void send_p1(uint8_t);
        static void send_p2(uint8_t);
        static uint8_t get();

    private:
        static bool dchannel;
        static constexpr int DATA = 0x60;
        static constexpr int PORT = 0x64;
    };
}