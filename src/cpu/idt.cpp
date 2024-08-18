#include <cpu/idt.hpp>
#include <cpu/gdt.hpp>
#include <drivers/ioapic.hpp>
#include <lib/logger.hpp>
#include <uacpi/kernel_api.h>
using namespace idt;

extern void *isr_table[256];
handler_t handlers[256 - 32];

desc idt_table[256];
idtr idt_idtr{
    256 * sizeof(idt::desc) - 1,
    reinterpret_cast<uintptr_t>(&idt_table)
};

void idt::init() {
    for (int i = 0; i < 256; i++)
        set(i, 0, isr_table[i]);
    asm volatile ("lidt %0; sti" : : "rm" (idt_idtr) : "memory");
    logger::info("IDT[INIT]", "Initialized IDT");
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

std::pair<handler_t&, uint8_t> idt::allocate_handler(uint8_t irq) {
    uint8_t vec = irq + 32;
    if (!handlers[irq]) return { handlers[irq], vec };
    logger::panic("IDT", "IDT entry %hhu is already reserved", vec);
}

std::pair<handler_t&, uint8_t> idt::allocate_handler() {
    for (uint8_t i = ioapic::initialized ? 0 : 0x10; i < 256 - 32; i++) // Skip the area reserved for hardware IRQs
        if (!handlers[i])
            return { handlers[i], i + 32 };
    logger::panic("IDT", "No more IDT entries available for use");
}

void idt::free_handler(uint8_t irq) {
    handlers[irq] = nullptr;
}