#include <cpu/idt.hpp>
#include <cpu/gdt.hpp>
#include <util/logger.hpp>
#include <uacpi/kernel_api.h>
using namespace idt;

extern void *isr_table[256];
Handler handlers[256 - 32];

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

std::pair<Handler&, uint8_t> idt::allocate_handler(uint8_t irq) {
    uint8_t vec = irq + 32;
    if (!handlers[irq]) return { handlers[irq], vec };
    logger::panic("IDT", "IDT entry %hhu is already reserved", vec);
}

std::pair<Handler&, uint8_t> idt::allocate_handler() {
    for (uint8_t i = 0; i < 256 - 32; i++)
        if (!handlers[i])
            return { handlers[i], i + 32 };
    logger::panic("IDT", "No more IDT entries available for use");
}

void idt::free_handler(uint8_t irq) {
    handlers[irq].reset();
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler func, uacpi_handle ctx, uacpi_handle *out_handle) {
    auto [handler, vec] = idt::allocate_handler(irq);
    handler = [func, ctx](cpu::status *status) {
        func(ctx);
        return status;
    };
    *reinterpret_cast<size_t*>(out_handle) = irq;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle handle) {
    idt::free_handler(*reinterpret_cast<size_t*>(handle));
    return UACPI_STATUS_OK;
}