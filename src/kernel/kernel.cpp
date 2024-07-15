#include <init.hpp>
#include <io/serial.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <mem/pmm.hpp>
#include <mem/mapper.hpp>
#include <drivers/framebuffer.hpp>
#include <mem/vmm.hpp>
#include <mem/heap.hpp>
#include <acpi/acpi.hpp>
#include <acpi/madt.hpp>
#include <common.h>

uint8_t CPU = 0;

template <uint16_t S>
void init_gdt(cpu::GDT<S>&);
void init_idt(cpu::IDT&);

extern "C" void __cxa_pure_virtual() { while(1); }

extern "C" void __attribute__((noreturn)) init_kernel(struct boot_info *bi) {
    call_constructors();
    io::SerialPort qemu{0x3F8};
    qemu.set_global();

    cpu::GDT<3> gdt;
    init_gdt(gdt);

    cpu::IDT idt;
    init_idt(idt);

    mem::PMM pmm{bi};
    mem::PTMapper mapper{bi->pml4, pmm};
    drivers::Framebuffer fb{bi, mapper, pmm};
    mem::VMM vmm{mem::VMM::Supervisor, bi->mem_pages, mapper, pmm};
    mem::Heap heap{vmm, PAGE_SIZE};
    acpi::RSDT rsdt{bi->rsdp};
    auto madt = rsdt.get_table<acpi::MADT>();
    while(1);
}

template <uint16_t S>
void init_gdt(cpu::GDT<S>& gdt) {
    gdt.set_entry(1, 0b10011011, 0b10);
    gdt.set_entry(2, 0b10010011, 0);
    gdt.load();
}

void init_idt(cpu::IDT& idt) {
    cpu::load_exceptions(idt);
    idt.load();
}
