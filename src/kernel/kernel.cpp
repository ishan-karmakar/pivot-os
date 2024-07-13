#include <boot.h>
#include <common.h>
#include <drivers/qemu.hpp>
#include <io/stdio.h>
#include <io/stdio.hpp>
#include <util/logger.h>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>
#include <mem/vmm.hpp>

uint8_t CPU = 0;

// Global constructors are not called on these
drivers::QEMUWriter qemu_writer;
cpu::GlobalDescriptorTable<3> gdt;
cpu::InterruptDescriptorTable idt;
mem::PhysicalMemoryManager pmm;
mem::PTMapper mapper;
mem::VirtualMemoryManager vmm;

void init_qemu();
void init_gdt();
void init_idt();
void init_pmm(struct boot_info*);
void init_mapper(struct boot_info*);
void init_vmm(struct boot_info*);

extern "C" void __cxa_pure_virtual() { while(1); }

extern "C" void __attribute__((noreturn)) init_kernel(struct boot_info *bi) {
    char_printer = io_char_printer;
    // init_qemu();
    // init_gdt();
    // init_idt();
    // init_pmm(bi);
    // init_mapper(bi);
    // init_vmm(bi);
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
    cpu::isr::load_exceptions(idt);
    idt.load();
}

void init_pmm(struct boot_info *bi) {
    pmm.init(bi);
}

void init_mapper(struct boot_info *bi) {
    mapper.init(bi->pml4, &pmm);
}

void init_vmm(struct boot_info *bi) {
    vmm.init(mem::vmm::Supervisor, bi->mem_pages, &mapper, &pmm);
}
