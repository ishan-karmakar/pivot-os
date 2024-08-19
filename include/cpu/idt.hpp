#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>
#include <cpu/cpu.hpp>
#include <lib/logger.hpp>

namespace idt {
    struct [[gnu::packed]] desc {
        uint16_t offset0;
        uint16_t segment_selector;
        uint8_t ist;
        uint8_t flags;
        uint16_t offset1;
        uint32_t offset2;
        uint32_t rsv;
    };

    struct [[gnu::packed]] idtr {
        uint16_t size;
        uintptr_t addr;
    };

    typedef std::function<cpu::status* (cpu::status*)> handler_t;

    void init();
    void load();
    void set(uint8_t, idt::desc);
    void set(uint8_t, uint8_t, void*);
    std::pair<handler_t&, uint8_t> allocate_handler();
    std::pair<handler_t&, uint8_t> allocate_handler(uint8_t);
    void free_handler(uint8_t);
}

