#include <boot.h>
#include <drivers/qemu.hpp>
#include <io/stdio.h>
#include <io/stdio.hpp>
#include <util/logger.h>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>

uint8_t CPU = 0;

// Global constructors are not called on these
drivers::QEMUWriter qemu_writer;
cpu::GlobalDescriptorTable<3> gdt;
cpu::InterruptDescriptorTable idt;

void init_qemu();
void init_gdt();
void init_idt();

extern "C" void __cxa_pure_virtual() { while(1); }

extern "C" void __attribute__((noreturn)) init_kernel(boot_info_t *bi) {
    char_printer = io_char_printer;
    init_qemu();
    init_gdt();
    init_idt();

    while(1);
}

void init_qemu() {
    qemu_writer.init();
    qemu_writer.set_global();
    log(Info, "QEMU", "Initialized QEMU serial output");
}

void init_gdt() {
    gdt.set_entry(1, 0b10011011, 0b10);
    gdt.set_entry(2, 0b10010011, 0);
    gdt.load();
}

void init_idt() {
}