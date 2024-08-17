#pragma once
#include <cstdint>

namespace lapic {
    extern std::size_t ms_ticks;

    void bsp_init();
    void ap_init();

    void write_reg(uint32_t, uint64_t);
    uint64_t read_reg(uint32_t);
    inline void eoi() { write_reg(0xB0, 0); }

    constexpr int SPURIOUS_OFF = 0xF0;
    constexpr int INITIAL_COUNT_OFF = 0x380;
    constexpr int CUR_COUNT_OFF = 0x390;
    constexpr int CONFIG_OFF = 0x3E0;
}
