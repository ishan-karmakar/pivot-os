#include <stdbool.h>
#include <cpuid.h>
#include <cpu/cpu.h>
#include <cpu/lapic.h>
#include <cpu/idt.h>
#include <cpu/ioapic.h>
#include <scheduler/scheduler.h>
#include <kernel/logging.h>
#include <mem/pmm.h>
#include <io/ports.h>
#include <kernel.h>

extern void hcf(void);
void disable_pic(void);

void init_lapic(void) {
    map_addr(KLAPIC.addr, KLAPIC.addr, KERNEL_PT_ENTRY, KMEM.pml4);

    uint64_t msr_output = rdmsr(IA32_APIC_BASE);
    uint32_t ignored, xApicLeaf = 0, x2ApicLeaf = 0;
    __get_cpuid(1, &ignored, &ignored, &x2ApicLeaf, &xApicLeaf);

    if (x2ApicLeaf & (1 << 21)) {
        KLAPIC.x2mode = 1;
        msr_output |= (1 << 10);
        wrmsr(IA32_APIC_BASE, msr_output);
    } else if (xApicLeaf & (1 << 9)) {
        KLAPIC.x2mode = -1;
        map_lapic(NULL);
    } else
        return log(Error, "LAPIC", "No LAPIC is supported by this CPU");
    
    if (!((msr_output >> APIC_GLOBAL_ENABLE_BIT)) & 1)
        return log(Error, "LAPIC", "APIC is disabled globally");

    write_apic_register(APIC_SPURIOUS_VEC_OFF, APIC_SOFTWARE_ENABLE | APIC_SPURIOUS_IDT_ENTRY);
    pmm_set_bit(KLAPIC.addr);
    disable_pic();
    log(Verbose, "LAPIC", "Disabled 8259 PIC");
    // APIC needs to be accessible from userspace
    IDT_SET_INT(APIC_PERIODIC_IDT_ENTRY, 3, apic_periodic_irq);
    IDT_SET_INT(APIC_ONESHOT_IDT_ENTRY, 0, apic_oneshot_irq);
    IDT_SET_INT(PIT_IDT_ENTRY, 0, pit_irq);
    IDT_SET_INT(APIC_SPURIOUS_IDT_ENTRY, 0, spurious_irq);
    log(Info, "LAPIC", "Initialized %sAPIC", KLAPIC.x2mode == 1 ? "x2" : "x");
}

void init_lapic_ap(void) {
    write_apic_register(APIC_SPURIOUS_VEC_OFF, APIC_SOFTWARE_ENABLE | APIC_SPURIOUS_IDT_ENTRY);
    log(Info, "LAPIC", "Initialized %sAPIC", KLAPIC.x2mode ? "x2" : "x");
}

void disable_pic(void) {
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    log(Verbose, "LAPIC", "Disabled PIC");
}

uint32_t read_apic_register(uint32_t reg_off) {
    if (KLAPIC.x2mode == 1)
        return (uint32_t) rdmsr((reg_off >> 4) + 0x800);
    else
        return *(volatile uint32_t*) (KLAPIC.addr + reg_off);
}

void write_apic_register(uint32_t reg_off, uint32_t val) {
    if (KLAPIC.x2mode == 1)
        wrmsr((reg_off >> 4) + 0x800, val);
    else
        *(volatile uint32_t*)(KLAPIC.addr + reg_off) = val;
}

bool apic_initialized(void) { return KLAPIC.x2mode; }

uint32_t get_apic_id(void) {
    if (KLAPIC.x2mode == 1)
        return read_apic_register(APIC_ID_OFF);
    return read_apic_register(APIC_ID_OFF) >> 24;
}

void delay(size_t num_ticks) {
    start_apic_timer(0, num_ticks, APIC_ONESHOT_IDT_ENTRY);
    while (!KLAPIC.oneshot_trig) asm ("pause");
    KLAPIC.oneshot_trig = false;
}

void calibrate_apic_timer(void) {
    set_irq(0, PIT_IDT_ENTRY, 0, 0, true);
    outb(PIT_MODE_COMMAND_REGISTER, 0b00110100);
    uint16_t counter = PIT_1_MS;
    outb(PIT_CHANNEL_0_DATA_PORT, counter & 0xFF);
    outb(PIT_CHANNEL_0_DATA_PORT, (counter >> 8) & 0xFF);

    write_apic_register(APIC_TIMER_INITIAL_COUNT_OFF, 0);
    write_apic_register(APIC_TIMER_CONFIG_OFF, APIC_TIMER_DIVIDER);

    KLAPIC.pit_ticks = 0;
    set_irq_mask(0, false);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_OFF, (uint32_t)-1);
    while(KLAPIC.pit_ticks < 500) asm ("pause");
    uint32_t current_apic_count = read_apic_register(APIC_TIMER_CURRENT_COUNT_OFF);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_OFF, 0);
    set_irq_mask(0, true);
    KLAPIC.pit_ticks = 0;

    uint32_t time_elapsed = ((uint32_t)-1) - current_apic_count;
    KLAPIC.ms_interval = time_elapsed / 500;
    log(Verbose, "LAPIC", "Measured %u ticks per ms, %u ticks per us", KLAPIC.ms_interval, (KLAPIC.ms_interval + 500) / 1000);
    log(Info, "LAPIC", "Calibrated APIC timer");
}

void start_apic_timer(uint32_t timer_mode, size_t initial_count, uint8_t idt_entry) {
    write_apic_register(APIC_TIMER_LVT_OFFSET, idt_entry | timer_mode);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_OFF, initial_count);
    write_apic_register(APIC_TIMER_CONFIG_OFF, APIC_TIMER_DIVIDER);

    asm ("sti");
}

cpu_status_t *apic_periodic_handler(cpu_status_t *status) {
    KLAPIC.apic_ticks++;
    status = schedule(status);
    APIC_EOI();
    return status;
}

cpu_status_t *apic_oneshot_handler(cpu_status_t *status) {
    KLAPIC.oneshot_trig = true;
    APIC_EOI();
    return status;
}

cpu_status_t *pit_handler(cpu_status_t *status) {
    KLAPIC.pit_ticks++;
    APIC_EOI();
    return status;
}

cpu_status_t *spurious_handler(cpu_status_t *status) { return status; }

void map_lapic(page_table_t pml4) {
    if (KLAPIC.x2mode == -1)
        map_addr(KLAPIC.addr, KLAPIC.addr, KERNEL_PT_ENTRY, pml4);
}
