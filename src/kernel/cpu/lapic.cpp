#include <cpu/lapic.hpp>
#include <cpu/cpu.hpp>
#include <util/logger.h>
#include <io/serial.hpp>
#include <io/pit.hpp>
#include <cstdint>
#include <cpuid.h>
#include <common.h>
using namespace cpu;

#define TIMER_DIV Div4

#define IA32_APIC_BASE 0x1B

#define SPURIOUS_VEC_OFF 0xF0
#define INITIAL_COUNT_OFF 0x380
#define CUR_COUNT_OFF 0x390
#define CONFIG_OFF 0x3E0

extern "C" void apic_periodic_irq();

uintptr_t LAPIC::lapic;
bool LAPIC::x2mode;
uint32_t LAPIC::ms_interval;

void LAPIC::init(mem::PTMapper& mapper, IDT& idt) {
    auto madt = acpi::ACPI::get_table<acpi::MADT>().value();
    uint64_t msr = rdmsr(IA32_APIC_BASE);
    if (!(msr & (1 << 11)))
        log(Warning, "LAPIC", "APIC is disabled globally");
    
    uint32_t ignored, xapic = 0, x2apic = 0;
    __get_cpuid(1, &ignored, &ignored, &x2apic, &xapic);

    if (x2apic & (1 << 21)) {
        x2mode = true;
        msr |= (1 << 10);
    } else if (xapic & (1 << 9)) {
        x2mode = false;
        lapic = VADDR(madt.table->lapic_addr);
        mapper.map(PADDR(lapic), lapic, KERNEL_PT_ENTRY);
        mem::PMM::set(PADDR(lapic));
    } else {
        log(Error, "LAPIC", "No LAPIC is supported by this processor");
        return;
    }

    wrmsr(IA32_APIC_BASE, msr);

    // Disable PIC
    io::outb(0x21, 0xFF);
    io::outb(0xA1, 0xFF);
    log(Verbose, "LAPIC", "Disabled 8259 PIC");

    write_reg(SPURIOUS_VEC_OFF, (1 << 8) | APIC_SPURIOUS_IDT_ENT);
    idt.set_entry(APIC_PERIODIC_IDT_ENT, 3, apic_periodic_irq);
    io::PIT::init(idt);

    log(Info, "LAPIC", "Initialized %sAPIC", x2mode ? "x2" : "x");
}

void LAPIC::calibrate() {
    asm volatile ("sti");
    IOAPIC::set_irq(0, PIT_IDT_ENT, 0, 0, true);
    io::PIT::cmd(false, 0b010, 0b11, 0);
    io::PIT::data(PIT_MS);

    write_reg(INITIAL_COUNT_OFF, 0);
    write_reg(CONFIG_OFF, TIMER_DIV);

    IOAPIC::set_mask(0, false);
    write_reg(INITIAL_COUNT_OFF, (uint32_t) - 1);
    while (io::PIT::ticks < 500) asm ("pause");
    uint32_t cur_ticks = read_reg(CUR_COUNT_OFF);
    IOAPIC::set_mask(0, true);
    io::PIT::ticks = 0;

    uint32_t time_elapsed = ((uint32_t)-1) - cur_ticks;
    ms_interval = time_elapsed / 500;
    log(Verbose, "LAPIC", "APIC ticks per ms: %u", ms_interval);
}

uint64_t LAPIC::read_reg(uint32_t off) {
    if (x2mode)
        return rdmsr((off >> 4) + 0x800);
    else
        return *(volatile uint32_t*) (lapic + off);
}

void LAPIC::write_reg(uint32_t off, uint64_t val) {
    if (x2mode)
        wrmsr((off >> 4) + 0x800, val);
    else
        *(volatile uint32_t*)(lapic + off) = val;
}

extern "C" cpu::cpu_status *apic_periodic_handler(cpu_status * const status) {
    return status;
}