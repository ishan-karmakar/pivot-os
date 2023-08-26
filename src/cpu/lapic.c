#include <cpu/lapic.h>
#include <cpu/cpu.h>
#include <mem/mem.h>
#include <kernel/logging.h>
#include <cpuid.h>
#include <io/ports.h>
#include <cpu/ioapic.h>
#include <drivers/framebuffer.h>
#include <stdbool.h>
#include <sys.h>
extern void hcf(void);

uint64_t apic_hh_address;
bool x2mode;
volatile uint64_t pit_ticks = 0;
volatile uint64_t apic_ticks = 0;
uint32_t apic_ticks_interval;

uint32_t read_apic_register(uint32_t);
void write_apic_register(uint32_t, uint32_t);
void disable_pic(void);

void init_apic(size_t mem_size) {
    uint64_t msr_output = rdmsr(IA32_APIC_BASE);
    uint32_t apic_base_address = msr_output & APIC_BASE_ADDRESS_MASK;
    if (apic_base_address == 0) {
        log(Error, "LAPIC", "Cannot determine apic base address");
        hcf();
    }
    apic_hh_address = apic_base_address + HIGHER_HALF_OFFSET;

    uint32_t ignored, xApicLeaf = 0, x2ApicLeaf = 0;
    __get_cpuid(1, &ignored, &ignored, &x2ApicLeaf, &xApicLeaf);

    if (x2ApicLeaf & (1 << 21)) {
        log(Info, "LAPIC", "x2APIC Available");
        x2mode = true;
        msr_output |= (1 << 10);
        wrmsr(IA32_APIC_BASE, msr_output);
    } else if (xApicLeaf & (1 << 9)) {
        log(Info, "LAPIC", "xAPIC Available");
        x2mode = false;
        map_addr(apic_base_address, apic_hh_address, PRESENT_BIT | WRITE_BIT);
    } else
        return log(Error, "LAPIC", "No LAPIC is supported by this CPU");
    
    if (!((msr_output >> APIC_GLOBAL_ENABLE_BIT) & 1))
        return log(Error, "LAPIC", "APIC is disabled globally");
    
    write_apic_register(APIC_SPURIOUS_VEC_REG_OFF, APIC_SOFTWARE_ENABLE | APIC_SPURIOUS_INTERRUPT);
    log(Info, "LAPIC", "Initialized APIC");
    if (apic_base_address < mem_size) {
        log(Verbose, "LAPIC", "APIC base address is in physical memory area");
        bitmap_set_bit_addr(apic_base_address);
    }
    disable_pic();
    log(Info, "LAPIC", "Disabled PIC");
}

void disable_pic(void) {
    outportb(PIC_COMMAND_MASTER, 0x11);
    outportb(PIC_COMMAND_SLAVE, 0x11);

    outportb(PIC_DATA_MASTER, 0x20);
    outportb(PIC_DATA_SLAVE, 0x28);

    outportb(PIC_DATA_MASTER, 4);
    outportb(PIC_DATA_SLAVE, 2);

    outportb(PIC_DATA_MASTER, 1);
    outportb(PIC_DATA_SLAVE, 1);

    outportb(PIC_DATA_MASTER, 0xFF);
    outportb(PIC_DATA_SLAVE, 0xFF);
}

inline uint32_t read_apic_register(uint32_t reg_off) {
    if (x2mode)
        return (uint32_t) rdmsr((reg_off >> 4) + 0x800);
    else
        return *(volatile uint32_t*)(apic_hh_address + reg_off);
}

inline void write_apic_register(uint32_t reg_off, uint32_t val) {
    if (x2mode)
        wrmsr((reg_off >> 4) + 0x800, val);
    else
        *(volatile uint32_t*)(apic_hh_address + reg_off) = val;
}

uint32_t bsp_id(void) {
    if (x2mode)
        return read_apic_register(APIC_ID_REG_OFF);
    return read_apic_register(APIC_ID_REG_OFF) >> 24;
}

// Timer will trigger interrupt every tenth of millisecond
void calibrate_apic_timer(uint8_t divider) {
    outportb(PIT_MODE_COMMAND_REGISTER, 0b00110100);
    uint16_t counter = PIT_1_MS;
    outportb(PIT_CHANNEL_0_DATA_PORT, counter & 0xFF);
    outportb(PIT_CHANNEL_0_DATA_PORT, (counter >> 8) & 0xFF);

    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, 0);
    write_apic_register(APIC_TIMER_CONFIG_OFF, divider);

    set_irq_mask(0x14, 0);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, (uint32_t)-1);
    while(pit_ticks < 500);
    uint32_t current_apic_count = read_apic_register(APIC_TIMER_CURRENT_COUNT_REG_OFF);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, 0);
    set_irq_mask(0x14, 1);
    
    uint32_t time_elapsed = ((uint32_t)-1) - current_apic_count;
    apic_ticks_interval = time_elapsed / 500;
    log(Verbose, "APIC", "Measured %u ticks per ms", apic_ticks_interval);
}

void mdelay(size_t ms) {
    size_t current_apic_ticks = apic_ticks;
    while ((apic_ticks - current_apic_ticks) < ms);
}

void udelay(size_t us) {
    size_t total_ticks = (apic_ticks_interval * us) / 1000;
    apic_ticks = 0;
    size_t initial_ticks = read_apic_register(APIC_TIMER_CURRENT_COUNT_REG_OFF);
    while (total_ticks > ((apic_ticks * apic_ticks_interval) + initial_ticks - read_apic_register(APIC_TIMER_CURRENT_COUNT_REG_OFF)));
    size_t t = read_apic_register(APIC_TIMER_CURRENT_COUNT_REG_OFF);
    log(Verbose, "TIMER", "%u, %u, %u", initial_ticks, t, initial_ticks - t);
}
// 156
// 6372 6027

void start_apic_timer(uint8_t divider) {
    calibrate_apic_timer(divider);
    write_apic_register(APIC_TIMER_LVT_OFFSET, 0x20000 | APIC_TIMER_IDT_ENTRY);
    write_apic_register(APIC_TIMER_INITIAL_COUNT_REG_OFF, apic_ticks_interval);
    write_apic_register(APIC_TIMER_CONFIG_OFF, divider);

    asm ("sti");
}