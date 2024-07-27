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
#include <frg/printf.hpp>
#include <limine.h>
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

extern void io_char_printer(char);

__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".requests.start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests.end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void test(const char *s, ...) {
    va_list args;
    va_start(args, s);
    frg::va_struct struc = {
        .arg_list = args,
        .args = nullptr,
        .num_args = 0
    };
    frg::printf_format(io_char_printer, s, );
    va_end(args);
}

extern "C" void __attribute__((noreturn)) init_kernel() {
    io::SerialPort qemu{0x3F8};
    qemu.set_global();
    cxxabi::call_constructors();
    mem::pmm::init();
    frg::printf_format(io_char_printer, "test", );
    // mem::mapper::init(bi->pml4);
    // mem::VMM vmm{mem::VMM::Supervisor, bi->mem_pages, *mem::kmapper};
    // mem::Heap heap{vmm, HEAP_SIZE};
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
