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
#include <init.hpp>

uint8_t CPU = 0;

template <uint16_t S>
void init_gdt(cpu::GlobalDescriptorTable<S>&);
void init_idt(cpu::InterruptDescriptorTable&);
void init_pmm(struct boot_info*);
void init_mapper(struct boot_info*);
void init_vmm(struct boot_info*);

extern "C" void __cxa_pure_virtual() { while(1); }

extern "C" void __attribute__((noreturn)) init_kernel(struct boot_info *bi) {
    call_constructors();
    drivers::QEMUWriter qemu_writer;
    cpu::GlobalDescriptorTable<3> gdt;
    init_gdt(gdt);

    cpu::InterruptDescriptorTable idt;
    init_idt(idt);

    mem::PhysicalMemoryManager pmm{bi};
    mem::PTMapper mapper{bi->pml4, pmm};
    mem::VirtualMemoryManager vmm{mem::vmm::Supervisor, bi->mem_pages, mapper, pmm};

    while(1);
}

template <uint16_t S>
void init_gdt(cpu::GlobalDescriptorTable<S>& gdt) {
    gdt.set_entry(1, 0b10011011, 0b10);
    gdt.set_entry(2, 0b10010011, 0);
    gdt.load();
}

void init_idt(cpu::InterruptDescriptorTable& idt) {
    cpu::isr::load_exceptions(idt);
    idt.load();
}

// void init_pmm(struct boot_info *bi) {
//     pmm.init(bi);
// }

// void init_mapper(struct boot_info *bi) {
//     mapper.init(bi->pml4, &pmm);
// }

// void init_vmm(struct boot_info *bi) {
//     vmm.init(mem::vmm::Supervisor, bi->mem_pages, &mapper, &pmm);
// }
