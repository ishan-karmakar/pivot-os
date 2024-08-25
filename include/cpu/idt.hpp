#pragma once
#include <cstdint>
#include <cstddef>
#include <frg/manual_box.hpp>
#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <vector>
#include <mem/heap.hpp>
#include <unordered_map>

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

    struct handler_t : std::vector<func_t> {
        void push_back(const func_t& v) {
            asm volatile ("cli");
            std::vector<func_t>::push_back(v);
            asm volatile ("sti");
        }

        void push_back(func_t&& v) {
            asm volatile ("cli");
            std::vector<func_t>::push_back(std::move(v));
            asm volatile ("sti");
        }
    };

    typedef std::unordered_map<unsigned int, handler_t> handlers_t;

    void init();
    void load();
    void set(const uint8_t&, idt::desc);
    void set(const uint8_t&, const uint8_t&, void*);
    uint8_t set_handler(func_t&&);

    extern handlers_t handlers;
}

