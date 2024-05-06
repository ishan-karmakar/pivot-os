#include <stdbool.h>
#include <cpuid.h>
#include <cpu/cpu.h>
#include <cpu/lapic.h>
#include <cpu/idt.h>
#include <cpu/ioapic.h>
#include <scheduler/scheduler.h>
#include <kernel/logging.h>
#include <mem/bitmap.h>
#include <mem/pmm.h>
#include <io/ports.h>
#include <sys.h>

extern void hcf(void);
void disable_pic(void);

uintptr_t apic_address;
bool x2mode;
uint32_t apic_ms_interval;
volatile size_t pit_ticks = 0, apic_ticks = 0;
volatile bool apic_triggered = false;

void init_lapic(void) {
    uint64_t msr_output = rdmsr(IA32_APIC_BASE);
    uint32_t apic_base_address = msr_output & APIC_BASE_ADDRESS_MASK;
    if (apic_base_address == 0) {
        log(Error, "LAPIC", "Cannot determine apic base address");
        hcf();
    }

    log(Verbose, "LAPIC", "APIC base address: %x", apic_base_address);

    apic_address = VADDR(apic_base_address);

    uint32_t ignored, xApicLeaf = 0, x2ApicLeaf = 0;
    __get_cpuid(1, &ignored, &ignored, &x2ApicLeaf, &xApicLeaf);

    if (x2ApicLeaf & (1 << 21)) {
        log(Verbose, "LAPIC", "x2APIC Available");
        x2mode = true;
        msr_output |= (1 << 10);
        wrmsr(IA32_APIC_BASE, msr_output);
    } else if (xApicLeaf & (1 << 9)) {
        log(Verbose, "LAPIC", "xAPIC Available");
        x2mode = false;
        map_lapic(NULL);
    } else
        return log(Error, "LAPIC", "No LAPIC is supported by this CPU");
    
    if (!((msr_output >> APIC_GLOBAL_ENABLE_BIT)) & 1)
        return log(Error, "LAPIC", "APIC is disabled globally");

    write_apic_register(APIC_SPURIOUS_VEC_REG_OFF, APIC_SOFTWARE_ENABLE | APIC_SPURIOUS_INTERRUPT);
    log(Info, "LAPIC", "Initialized APIC");
    bitmap_rsv_area(apic_base_address, 1);
    disable_pic();
    log(Info, "LAPIC", "Disabled PIC");
    // APIC needs to be accessible from userspace
    IDT_SET_INT(APIC_TIMER_PERIODIC_IDT_ENTRY, 3, apic_periodic_irq);
    IDT_SET_INT(APIC_TIMER_ONESHOT_IDT_ENTRY, 0, apic_oneshot_irq);
    IDT_SET_INT(PIT_TIMER_IDT_ENTRY, 0, pit_irq);
}

void disable_pic(void) {
    outb(PIC_COMMAND_MASTER, 0x11);
    outb(PIC_COMMAND_SLAVE, 0x11);

    outb(PIC_DATA_MASTER, 0x20);
    outb(PIC_DATA_SLAVE, 0x28);

    outb(PIC_DATA_MASTER, 4);
    outb(PIC_DATA_SLAVE, 2);

    outb(PIC_DATA_MASTER, 1);
    outb(PIC_DATA_SLAVE, 1);

    outb(PIC_DATA_MASTER, 0xFF);
    outb(PIC_DATA_SLAVE, 0xFF);
}

uint32_t read_apic_register(uint32_t reg_off) {
    if (x2mode)
        return (uint32_t) rdmsr((reg_off >> 4) + 0x800);
    else
        return *(volatile uint32_t*) (apic_address + reg_off);
}

void write_apic_register(uint32_t reg_off, uint32_t val) {
    if (x2mode)
        wrmsr((reg_off >> 4) + 0x800, val);
    else
        *(volatile uint32_t*)(apic_address + reg_off) = val;
}

uint32_t get_apic_id(void) {
    if (x2mode)
        return read_apic_register(APIC_ID_REG_OFF);
    return read_apic_register(APIC_ID_REG_OFF) >> 24;
}

void delay(size_t num_ticks) {
    start_apic_timer(0, num_ticks, APIC_TIMER_ONESHOT_IDT_ENTRY);
    while (!apic_triggered) asm ("pause");
    apic_triggered = false;
}

void calibrate_apic_timer(void) {
    set_irq(2, 34, 0, 0, true);
    asm ("sti");
    outb(PIT_MODE_COMMAND_REGISTER, 0b00110100);
    uint16_t counter = PIT_1_MS;
    outb(PIT_CHANNEL_0_DATA_PORT, counter & 0xFF);
    outb(PIT_CHANNEL_0_DATA_PORT, (counter >> 8) & 0xFF);

    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, 0);
    write_apic_register(APIC_TIMER_CONFIG_OFF, APIC_TIMER_DIVIDER);

    set_irq_mask(2, false);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, (uint32_t)-1);
    while(pit_ticks < 500);
    uint32_t current_apic_count = read_apic_register(APIC_TIMER_CURRENT_COUNT_REG_OFF);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, 0);
    set_irq_mask(2, true);
    
    uint32_t time_elapsed = ((uint32_t)-1) - current_apic_count;
    apic_ms_interval = time_elapsed / 500;
    log(Debug, "APIC", "Measured %u ticks per ms, %u ticks per us", apic_ms_interval, (apic_ms_interval + 500) / 1000);
    log(Info, "APIC", "Calibrated APIC timer");
}

void start_apic_timer(uint32_t timer_mode, size_t initial_count, uint8_t idt_entry) {
    write_apic_register(APIC_TIMER_LVT_OFFSET, idt_entry | timer_mode);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, initial_count);
    write_apic_register(APIC_TIMER_CONFIG_OFF, APIC_TIMER_DIVIDER);

    asm ("sti");
}

cpu_status_t *apic_periodic_handler(cpu_status_t *status) {
    apic_ticks++;
    status = schedule(status);
    APIC_EOI();
    return status;
}

cpu_status_t *apic_oneshot_handler(cpu_status_t *status) {
    apic_triggered = true;
    APIC_EOI();
    return status;
}

cpu_status_t *pit_handler(cpu_status_t *status) {
    pit_ticks++;
    APIC_EOI();
    return status;
}

void map_lapic(uint64_t *pml4) {
    if (!x2mode)
        map_addr(PADDR(apic_address), apic_address, KERNEL_PT_ENTRY, pml4);
}
