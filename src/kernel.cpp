#include <misc/cxxabi.hpp>
#include <mem/heap.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <mem/mapper.hpp>
#include <cpu/cpu.hpp>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <cpu/smp.hpp>
#include <cpu/tss.hpp>
#include <drivers/acpi.hpp>
#include <drivers/term.hpp>
#include <drivers/rtc.hpp>
#include <drivers/pic.hpp>
#include <drivers/pit.hpp>
#include <drivers/lapic.hpp>
#include <drivers/ioapic.hpp>
#include <lib/syscall.hpp>
#include <io/serial.hpp>
#include <lib/logger.hpp>
#include <lib/scheduler.hpp>
#include <frg/manual_box.hpp>
#include <limine.h>
#include <cstdlib>

extern void io_char_printer(char);

__attribute__((used, section(".requests")))
static LIMINE_BASE_REVISION(2);

__attribute__((used, section(".requests.start")))
static LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests.end")))
static LIMINE_REQUESTS_END_MARKER;

frg::manual_box<io::serial_port> qemu;

void kmain() {
    logger::info("KMAIN", "Entered kmain");
    proc::sleep(1000);
    logger::info("KMAIN", "After sleep");
}

extern "C" [[noreturn]] void kinit() {
    asm volatile ("cli");
    cpu::set_kgs(0);
    qemu.initialize(0x3F8);
    io::writer = qemu.get();
    cpu::init();
    gdt::early_init();
    idt::init();
    pmm::init();
    mapper::init();
    vmm::init();
    heap::init();
    cxxabi::call_constructors();
    smp::early_init();
    smp::this_cpu()->fpu_data = operator new(cpu::fpu_size);
    gdt::init();
    term::init();
    pic::init();
    pit::init();
    asm volatile ("sti");
    pit::start(pit::MS_TICKS);
    lapic::bsp_init();
    acpi::init();
    tss::init();
    tss::set_rsp0();
    rtc::init();
    syscalls::init();
    // smp::init();
    scheduler::init();
    auto kernel_proc = new scheduler::process{"kernel", kmain, true};
    kernel_proc->enqueue();
    scheduler::start();

    // drivers::PS2::init();
    // drivers::Keyboard::init(idt);
    while(1);
}

void abort() { cpu::hcf(); }
