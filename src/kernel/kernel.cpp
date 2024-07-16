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
#include <libc/string.h>
#include <cpu/tss.hpp>
#include <cpu/lapic.hpp>
#include <cpu/ioapic.hpp>
#include <common.h>
#define STACK_CHK_GUARD 0x595e9fbd94fda766

uint8_t CPU = 0;
cpu::GDT::gdt_desc initial_gdt[3];
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

cpu::GDT init_sgdt();
cpu::GDT init_hgdt(cpu::GDT&, mem::Heap&, acpi::ACPI&);

extern "C" void __attribute__((noreturn)) init_kernel(boot_info *bi) {
    call_constructors();
    io::SerialPort qemu{0x3F8};
    qemu.set_global();

    cpu::GDT sgdt{init_sgdt()};
    sgdt.load();

    cpu::IDT idt;
    cpu::load_exceptions(idt);
    idt.load();

    mem::PMM pmm{bi};
    mem::PTMapper mapper{bi->pml4, pmm};
    drivers::Framebuffer fb{bi, mapper, pmm};
    mem::VMM vmm{mem::VMM::Supervisor, bi->mem_pages, mapper, pmm};
    mem::Heap heap{vmm, PAGE_SIZE};
    acpi::ACPI rsdt{bi->rsdp};
    cpu::GDT hgdt{init_hgdt(sgdt, heap, rsdt)};
    cpu::TSS tss{hgdt, heap};
    tss.set_rsp0();
    cpu::LAPIC lapic{mapper, pmm, idt, rsdt.get_table<acpi::MADT>().value()};
    cpu::IOAPIC ioapic{mapper, pmm, rsdt.get_table<acpi::MADT>().value()};
    lapic.calibrate(ioapic);
    while(1);
}

cpu::GDT init_sgdt() {
    cpu::GDT sgdt{initial_gdt};
    sgdt.set_entry(1, 0b10011011, 0b10);
    sgdt.set_entry(2, 0b10010011, 0);
    return sgdt;
}

cpu::GDT init_hgdt(cpu::GDT& old, mem::Heap& heap, acpi::ACPI& acpi) {
    auto madt = acpi.get_table<acpi::MADT>();
    size_t num_cpus = 0;
    if (!madt.has_value())
        log(Warning, "ACPI", "Could not find MADT");
    for (auto iter = madt.value().iter<acpi::MADT::lapic>(); iter; ++iter, num_cpus++);
    log(Info, "KERNEL", "Number of CPUs: %u", num_cpus);
    auto heap_gdt = reinterpret_cast<cpu::GDT::gdt_desc*>(heap.alloc((5 + num_cpus * 2) * sizeof(cpu::GDT::gdt_desc)));
    cpu::GDT gdt{heap_gdt};
    gdt = old;
    return gdt;
}

[[noreturn]]
extern "C" void __cxa_pure_virtual() {
    log(Error, "KERNEL", "Could not find virtual function");
    abort();
}

[[noreturn]]
extern "C" void __stack_chk_fail() {
    log(Error, "KERNEL", "Detected stack smashing");
    abort();
}

extern "C" void abort() {
    log(Error, "KERNEL", "Aborting code");
    while(1);
}