#include <cpu/idt.hpp>
#include <cpu/isr.hpp>
#include <util/logger.hpp>
#include <uacpi/kernel_api.h>
using namespace idt;

frg::manual_box<IDT> idt::kidt;

void idt::init() {
    kidt.initialize();
    load_exceptions();
    kidt->load();
    logger::info("IDT[INIT]", "Loaded IDT");
}

void IDT::set_entry(uint8_t idx, idt::desc desc) {
    this->idt[idx] = desc;
}

void IDT::set_entry(uint8_t idx, uint8_t ring, void *handler) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(handler);
    set_entry(idx, {
        static_cast<uint16_t>(addr),
        0x8,
        0,
        static_cast<uint8_t>(0x8E | (ring << 5)),
        static_cast<uint16_t>(addr >> 16),
        static_cast<uint32_t>(addr >> 32),
        0
    });
}

void IDT::load() const {
    asm volatile ("lidt %0" : : "rm" (idtr));
}
