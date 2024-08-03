// #include <kernel.hpp>
#include <misc/cxxabi.hpp>
#include <mem/heap.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <mem/mapper.hpp>
#include <cpu/cpu.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
// #include <cpu/smp.hpp>
// #include <drivers/acpi.hpp>
#include <drivers/framebuffer.hpp>
#include <io/serial.hpp>
#include <util/logger.hpp>
#include <frg/manual_box.hpp>
#include <limine.h>
#include <cstdlib>

uint8_t CPU = 0;

extern "C" void frg_log(const char *s) {
    printf("%s\n", s);
}

extern "C" void frg_panic(const char *s) {
    frg_log(s);
    abort();
}

extern void io_char_printer(char);

__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".requests.start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests.end")))
static volatile LIMINE_REQUESTS_END_MARKER;

frg::manual_box<io::SerialPort> qemu;

extern "C" void init_kernel() {
    qemu.initialize(0x3F8);
    io::writer = qemu.get();
    cpu::gdt::early_init();
    cpu::idt::init();
    mem::pmm::init();
    mem::mapper::init();
    mem::vmm::init();
    // mem::heap::init();
    // cxxabi::call_constructors();
    // cpu::gdt::init();
    // drivers::fb::init();
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
