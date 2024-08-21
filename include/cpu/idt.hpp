#pragma once
#include <cstdint>
#include <cstddef>
#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <frg/vector.hpp>
#include <frg/hash_map.hpp>
#include <mem/heap.hpp>

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

    typedef std::function<cpu::status* (cpu::status*)> func_t;
    typedef frg::vector<func_t, heap::allocator_t> handler_t;
    typedef frg::hash_map<uint8_t, handler_t, frg::hash<unsigned int>, heap::allocator_t> handlers_t;

    void init();
    void load();
    void set(uint8_t, idt::desc);
    void set(uint8_t, uint8_t, void*);
    uint8_t set_handler(func_t&&);
    std::size_t set_handler(uint8_t, func_t&&);
    void free_handler(uint8_t, std::size_t);
    handlers_t& handlers();
}

