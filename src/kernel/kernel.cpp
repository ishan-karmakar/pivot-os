#include <common.h>
#include <misc/cxxabi.hpp>
#include <mem/heap.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <mem/mapper.hpp>
#include <cpu/cpu.hpp>
#include <cpu/smp.hpp>
#include <drivers/acpi.hpp>
#include <drivers/framebuffer.hpp>
#include <io/serial.hpp>
#include <util/logger.h>
#include <frg/slab.hpp>
#include <frg/manual_box.hpp>
#include <frg/spinlock.hpp>
#include <cstdlib>

uint8_t CPU = 0;
uintptr_t __stack_chk_guard = 0x595e9fbd94fda766;

extern "C" void frg_log(const char *s) {
    printf("%s\n", s);
}

extern "C" void frg_panic(const char *s) {
    frg_log(s);
    abort();
}

extern "C" void __attribute__((noreturn)) init_kernel(boot_info *bi) {
    io::SerialPort qemu{0x3F8};
    qemu.set_global();
    cxxabi::call_constructors();
    mem::pmm::init(bi);
    mem::mapper::init(bi->pml4);
    mem::VMM vmm{mem::VMM::Supervisor, bi->mem_pages, *mem::kmapper};
    mem::Heap heap{vmm, HEAP_SIZE};
    // mem::kheap = &heap;
    // drivers::Framebuffer fb{bi, mapper};
    // drivers::acpi::init(bi);
    // cpu::smp::init_bsp();
    // drivers::IOAPIC::init();
    // cpu::LAPIC::init(idt);
    // drivers::PIT::init(idt);

    // acpi::ACPI::init(bi->rsdp);

    // cpu::GDT hgdt{init_hgdt(sgdt)};
    
    // cpu::TSS tss{hgdt};
    // tss.set_rsp0();

    // drivers::IOAPIC::init(mapper);
    // cpu::LAPIC::init(mapper, idt);
    // cpu::LAPIC::calibrate();

    // drivers::RTC::init(idt);
    // drivers::PS2::init();
    // drivers::Keyboard::init(idt);
    while(1);
}

void abort() { cpu::hcf(); }
