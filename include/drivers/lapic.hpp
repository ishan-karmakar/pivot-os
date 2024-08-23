#pragma once
#include <cstdint>
#include <atomic>

namespace lapic {
    extern std::size_t ms_ticks;
    extern std::atomic_size_t ticks;
    extern bool initialized;

    enum timer_mode {
        Oneshot,
        Periodic,
        TSCDeadline
    };

    void bsp_init();
    void ap_init();
    void start(std::size_t);

    void write_reg(uint32_t, uint64_t);
    uint64_t read_reg(uint32_t);
    inline void eoi() { write_reg(0xB0, 0); }

    extern uint8_t timer_vec;
}
