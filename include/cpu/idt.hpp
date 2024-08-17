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

    class Handler {
    private:
        std::function<cpu::status* (cpu::status*)> handler;

    public:
        bool operator=(decltype(handler) f) {
            if (handler) return false;
            handler = f;
            return true;
        }

        operator bool() { return static_cast<bool>(handler); }
        cpu::status *operator()(cpu::status *status) { return handler(status); }
        void reset() { handler = nullptr; }
    };

    void init();
    void set(uint8_t, idt::desc);
    void set(uint8_t, uint8_t, void*);
    std::pair<Handler&, uint8_t> allocate_handler();
    std::pair<Handler&, uint8_t> allocate_handler(uint8_t);
    void free_handler(uint8_t);
}

