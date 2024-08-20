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

void handlers() {
    static frg::hash_map<uint8_t, handler_t, frg::hash<unsigned int>, heap::allocator_t> handlers{
        frg::hash<unsigned int>{},
        heap::allocator()
    };
    return handlers;
}

void idt::set_handler(uint8_t irq, func_t&& f) {
    handlers[irq].push_back(f);
}

uint8_t idt::set_handler(func_t&& f) {
    for (uint8_t i = ioapic::initialized ? 0 : 0x10; i < 256 - 32; i++) // Skip the area reserved for hardware IRQs
        if (!handlers[i].size()) {
            handlers[i].push_back(f);
            return i;
        }
    logger::panic("IDT", "No more IDT entries available for use");
}

void idt::free_handler(uint8_t irq) {
    handlers[irq].clear();
}