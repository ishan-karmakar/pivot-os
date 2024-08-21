#include <cpu/idt.hpp>
#include <cpu/gdt.hpp>
#include <drivers/ioapic.hpp>
#include <lib/logger.hpp>
#include <uacpi/kernel_api.h>
#include <frg/hash_map.hpp>
using namespace idt;

extern void *isr_table[256];

desc idt_table[256];
idtr idt_idtr{
    256 * sizeof(idt::desc) - 1,
    reinterpret_cast<uintptr_t>(&idt_table)
};

void idt::init() {
    for (int i = 0; i < 256; i++)
        set(i, 0, isr_table[i]);
    load();
    logger::info("IDT[INIT]", "Initialized IDT");
}

void idt::load() {
    asm volatile ("lidt %0; sti" : : "rm" (idt_idtr) : "memory");
}

void idt::set(uint8_t idx, idt::desc desc) {
    idt_table[idx] = desc;
}

void idt::set(uint8_t idx, uint8_t ring, void *handler) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(handler);
    set(idx, {
        static_cast<uint16_t>(addr),
        gdt::KCODE,
        0,
        static_cast<uint8_t>(0x8E | (ring << 5)),
        static_cast<uint16_t>(addr >> 16),
        static_cast<uint32_t>(addr >> 32),
        0
    });
}

handlers_t& idt::handlers() {
    static handlers_t h{{}, heap::allocator()};
    return h;
}

std::size_t idt::set_handler(uint8_t irq, func_t&& f) {
    auto& h = handlers()[irq];
    for (std::size_t i = 0; i < h.size(); i++)
        if (!h[i]) {
            h[i] = f;
            return i;
        }
    h.push_back(f);
    return h.size() - 1;
}

uint8_t idt::set_handler(func_t&& f) {
    auto& h = handlers();
    for (uint8_t i = ioapic::initialized ? 0 : 0x10; i < 256 - 32; i++) // Skip the area reserved for hardware IRQs
        if (!h[i].size()) {
            h[i].push_back(f);
            return i;
        }
    logger::panic("IDT", "No more IDT entries available for use");
}

// frg::vector doesn't provide an erase method, so the best we can do is set function to nullptr
void idt::free_handler(uint8_t irq, std::size_t idx) {
    handlers()[irq][idx] = nullptr;
}