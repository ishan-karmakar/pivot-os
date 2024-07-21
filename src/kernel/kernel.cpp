#include <common.h>
#include <init.hpp>
#include <mem/heap.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <mem/mapper.hpp>
#include <cpu/lapic.hpp>
#include <cpu/gdt.hpp>
#include <cpu/tss.hpp>
#include <cpu/cpu.hpp>
#include <cpu/idt.hpp>
#include <drivers/ioapic.hpp>
#include <drivers/framebuffer.hpp>
#include <drivers/rtc.hpp>
#include <drivers/keyboard.hpp>
#include <drivers/ps2.hpp>
#include <io/serial.hpp>
#include <util/logger.h>

uint8_t CPU = 0;
cpu::GDT::gdt_desc initial_gdt[3];
uintptr_t __stack_chk_guard = 0x595e9fbd94fda766;

cpu::GDT init_sgdt();
cpu::GDT init_hgdt(cpu::GDT&);

extern "C" void __attribute__((noreturn)) init_kernel(boot_info *bi) {
    call_constructors();
    io::SerialPort qemu{0x3F8};
    qemu.set_global();

    cpu::GDT sgdt{init_sgdt()};
    sgdt.load();

    cpu::IDT idt;
    cpu::load_exceptions(idt);
    idt.load();

    mem::PMM::init(bi);

    mem::PTMapper mapper{bi->pml4};
    drivers::Framebuffer fb{bi, mapper};
    mem::VMM vmm{mem::VMM::Supervisor, bi->mem_pages, mapper};
    mem::Heap heap{vmm, HEAP_SIZE};
    mem::kheap = &heap;

    acpi::ACPI::init(bi->rsdp);

    cpu::GDT hgdt{init_hgdt(sgdt)};
    
    cpu::TSS tss{hgdt};
    tss.set_rsp0();

    drivers::IOAPIC::init(mapper);
    cpu::LAPIC::init(mapper, idt);
    cpu::LAPIC::calibrate();

    drivers::RTC::init(idt);
    drivers::PS2::init();
    // drivers::Keyboard::init(idt);
    while(1);
}

cpu::GDT init_sgdt() {
    cpu::GDT sgdt{initial_gdt};
    sgdt.set_entry(1, 0b10011011, 0b10);
    sgdt.set_entry(2, 0b10010011, 0);
    return sgdt;
}

cpu::GDT init_hgdt(cpu::GDT& old) {
    auto madt = acpi::ACPI::get_table<acpi::MADT>();
    uint8_t num_cpus = 0;
    if (!madt.has_value())
        log(Warning, "ACPI", "Could not find MADT");
    for (auto iter = madt.value().iter<acpi::MADT::lapic>(); iter; ++iter, num_cpus++);
    log(Info, "KERNEL", "Number of CPUs: %hhu", num_cpus);
    auto heap_gdt = reinterpret_cast<cpu::GDT::gdt_desc*>(operator new((5 + num_cpus * 2) * sizeof(cpu::GDT::gdt_desc)));
    cpu::GDT gdt{heap_gdt};
    gdt = old;
    gdt.set_entry(3, 0b11111011, 0b10);
    gdt.set_entry(4, 0b11110011, 0);
    return gdt;
}

[[noreturn]]
extern "C" void __stack_chk_fail() {
    log(Error, "KERNEL", "Detected stack smashing");
    abort();
}

[[noreturn]]
extern "C" void __cxa_pure_virtual() {
    log(Error, "KERNEL", "Could not find virtual function");
    abort();
}

extern "C" int __cxa_atexit(void*, void*, void*) {
    return 0;
}

void abort() { cpu::hcf(); }
