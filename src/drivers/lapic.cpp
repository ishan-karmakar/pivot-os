#include <drivers/lapic.hpp>
#include <cpu/cpu.hpp>
#include <drivers/madt.hpp>
#include <drivers/acpi.hpp>
#include <util/logger.hpp>
#include <cpuid.h>
#include <kernel.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <drivers/pit.hpp>
#include <drivers/ioapic.hpp>
#include <cpu/idt.hpp>
#include <io/serial.hpp>
#include <uacpi/kernel_api.h>
using namespace drivers;

uintptr_t LAPIC::lapic;
bool LAPIC::x2mode;
uint32_t LAPIC::ms_interval;
bool LAPIC::initialized = false;

void LAPIC::bsp_init() {
    if (initialized) return;
    uint64_t msr = cpu::rdmsr(IA32_APIC_BASE);
    logger::assert(msr & (1 << 11), "LAPIC[INIT]", "APIC is disabled globally");
    
    uint32_t ignored, xapic = 0, x2apic = 0;
    __get_cpuid(1, &ignored, &ignored, &x2apic, &xapic);

    if (x2apic & (1 << 21)) {
        x2mode = true;
        msr |= (1 << 10);
    } else if (xapic & (1 << 9)) {
        x2mode = false;
        // auto madt = drivers::acpi::get_table<drivers::MADT>();
        // lapic = VADDR(madt.table->local_interrupt_controller_address);
        // mem::kmapper->map(PADDR(lapic), lapic, KERNEL_PT_ENTRY);
        // mem::PMM::set(PADDR(lapic));
    } else
        logger::panic("LAPIC[INIT]", "No LAPIC is supported by the processor");

    cpu::wrmsr(IA32_APIC_BASE, msr);

    // Disable PIC
    // TODO: Move this to a separate PIC file and maybe support legacy PIC
    io::outb(0x21, 0xFF);
    io::outb(0xA1, 0xFF);
    logger::verbose("LAPIC", "Disabled 8259 PIC");

    write_reg(SPURIOUS_OFF, (1 << 8) | SPURIOUS_IDT_ENT);
    // idt.set_entry(PERIODIC_IDT_ENT, 3, (void*) &periodic_irq);
    // idt.set_entry(SPURIOUS_IDT_ENT, 0, spurious_irq);

    logger::info("LAPIC[INIT]", "Initialized %sAPIC", x2mode ? "x2" : "x");
    initialized = true;
}

void LAPIC::calibrate() {
    asm volatile ("sti");
    write_reg(INITIAL_COUNT_OFF, 0);
    write_reg(CONFIG_OFF, TDIV);

    drivers::IOAPIC::set_mask(0, false);
    write_reg(INITIAL_COUNT_OFF, (uint32_t) - 1);
    while (drivers::PIT::ticks < 500) asm ("pause");
    uint32_t cur_ticks = read_reg(CUR_COUNT_OFF);
    drivers::IOAPIC::set_mask(0, true);
    drivers::PIT::ticks = 0;

    uint32_t time_elapsed = ((uint32_t) - 1) - cur_ticks;
    ms_interval = time_elapsed / 500;
    logger::verbose("LAPIC", "APIC ticks per ms: %u", ms_interval);
}

uint64_t LAPIC::read_reg(uint32_t off) {
    if (x2mode)
        return cpu::rdmsr((off >> 4) + 0x800);
    else
        return *(volatile uint32_t*) (lapic + off);
}

void LAPIC::write_reg(uint32_t off, uint64_t val) {
    if (x2mode)
        cpu::wrmsr((off >> 4) + 0x800, val);
    else
        *(volatile uint32_t*)(lapic + off) = val;
}

extern "C" cpu::status *periodic_handler(cpu::status *status) {
    // LAPIC::eoi();
    return status;
}

extern "C" cpu::status *spurious_handler(cpu::status *status) {
    return status;
}

void uacpi_kernel_sleep(uacpi_u64) {
    logger::info("uACPI", "uACPI requested to sleep");
}

uacpi_u64 uacpi_kernel_get_ticks() {
    logger::info("uACPI", "uACPI requested timer ticks");
    return 0;
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