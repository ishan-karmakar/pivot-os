#include <drivers/lapic.hpp>
#include <drivers/pic.hpp>
#include <cpu/cpu.hpp>
#include <cpuid.h>
#include <lib/logger.hpp>
#include <drivers/madt.hpp>
#include <cpu/idt.hpp>
#include <mem/mapper.hpp>
#include <drivers/interrupts.hpp>
#include <drivers/pit.hpp>
#include <lib/timer.hpp>
#include <uacpi/kernel_api.h>
using namespace lapic;

constexpr int TDIV1 = 0xB;
constexpr int TDIV2 = 0;
constexpr int TDIV4 = 1;
constexpr int TDIV8 = 2;
constexpr int TDIV16 = 3;
constexpr int TDIV32 = 8;
constexpr int TDIV64 = 9;
constexpr int TDIV128 = 0xA;

constexpr int TDIV = TDIV4;

constexpr int IA32_APIC_BASE = 0x1B;
bool x2mode;
static uintptr_t addr;
std::size_t lapic::ms_ticks = 0;

void calibrate();

void lapic::bsp_init() {
    uint64_t msr = cpu::rdmsr(IA32_APIC_BASE);
    logger::assert(msr & (1 << 11), "LAPIC[INIT]", "APIC is disabled globally");
    
    uint32_t ignored, xapic = 0, x2apic = 0;
    __get_cpuid(1, &ignored, &ignored, &x2apic, &xapic);

    if (x2apic & (1 << 21)) {
        x2mode = true;
        msr |= (1 << 10);
    } else if (xapic & (1 << 9)) {
        x2mode = false;
        auto madt = acpi::get_table<acpi::MADT>(ACPI_MADT_SIGNATURE);
        addr = madt.table->local_interrupt_controller_address;
        mapper::kmapper->map(addr, addr, mapper::KERNEL_ENTRY);
    } else
        logger::panic("LAPIC[INIT]", "No LAPIC is supported by the processor");

    cpu::wrmsr(IA32_APIC_BASE, msr);

    auto [spurious_handler, spurious_vector] = idt::allocate_handler();
    auto [periodic_handler, periodic_vector] = idt::allocate_handler();
    spurious_handler = [](cpu::status *status) { return status; };
    periodic_handler = [](cpu::status *status) {
        interrupts::eoi(0);
        return status;
    };
    write_reg(SPURIOUS_OFF, (1 << 8) | spurious_vector);
    // idt.set_entry(PERIODIC_IDT_ENT, 3, (void*) &periodic_irq);
    // idt.set_entry(SPURIOUS_IDT_ENT, 0, spurious_irq);

    logger::info("LAPIC[INIT]", "Initialized %sAPIC", x2mode ? "x2" : "x");

    calibrate();
}

void calibrate() {
    asm volatile ("sti");
    write_reg(INITIAL_COUNT_OFF, 0);
    write_reg(CONFIG_OFF, TDIV);

    pit::start(pit::MS_TICKS);
    write_reg(INITIAL_COUNT_OFF, (uint32_t) - 1);
    timer::sleep(500);
    uint32_t cur_ticks = read_reg(CUR_COUNT_OFF);
    pit::stop();

    uint32_t time_elapsed = ((uint32_t) - 1) - cur_ticks;
    ms_ticks = time_elapsed / 500;
    logger::verbose("LAPIC", "APIC ticks per ms: %u", ms_ticks);
}

uint64_t lapic::read_reg(uint32_t off) {
    if (x2mode)
        return cpu::rdmsr((off >> 4) + 0x800);
    else
        return *(volatile uint32_t*) (addr + off);
}

void lapic::write_reg(uint32_t off, uint64_t val) {
    if (x2mode)
        cpu::wrmsr((off >> 4) + 0x800, val);
    else
        *(volatile uint32_t*)(addr + off) = val;
}

void uacpi_kernel_signal_event(uacpi_handle) {
    logger::info("uACPI", "uACPI requested to signal event");
}

void uacpi_kernel_reset_event(uacpi_handle) {
    logger::info("uACPI", "uACPI requested to reset event");
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) {
    logger::info("uACPI", "uACPI requested to wait for event");
    return UACPI_TRUE;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*) {
    return UACPI_STATUS_UNIMPLEMENTED;
}