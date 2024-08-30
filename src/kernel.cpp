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
#include <drivers/pci.hpp>
#include <lib/syscall.hpp>
#include <io/serial.hpp>
#include <lib/logger.hpp>
#include <lib/scheduler.hpp>
#include <frg/manual_box.hpp>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#include <uacpi/notify.h>
#include <limine.h>
#include <assert.h>
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
    logger::info("SMP", "%lu", smp::this_cpu()->id);
    // assert(uacpi_likely_success(uacpi_namespace_initialize()));
    // assert(uacpi_likely_success(uacpi_find_devices("PNP0C0C", [](void*, uacpi_namespace_node *node) -> uacpi_ns_iteration_decision {
    //     assert(uacpi_likely_success(uacpi_install_notify_handler(node, [](void*, uacpi_namespace_node*, uint64_t v) -> uacpi_status {
    //         if (v == 0x80)
    //             acpi::shutdown();
    //         return UACPI_STATUS_OK;
    //     }, nullptr)));
    //     return UACPI_NS_ITERATION_DECISION_CONTINUE;
    // }, nullptr)));
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
    gdt::init();
    term::init();
    pic::init();
    pit::init();
    asm volatile ("sti");
    pit::start(pit::MS_TICKS);
    lapic::bsp_init();
    acpi::init();
    tss::init();
    // rtc::init(); // TODO: Move to a module
    syscalls::init();
    smp::init();
    scheduler::init();
    tss::set_rsp0();
    auto kernel_proc = new proc::process{kmain, true, *vmm::kvmm, *heap::pool};
    kernel_proc->cpu = 1;
    kernel_proc->enqueue();
    scheduler::start();
    while(1);
}

void abort() { cpu::hcf(); }
