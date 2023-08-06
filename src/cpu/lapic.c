#include <cpu/lapic.h>
#include <cpu/cpu.h>
#include <mem/mem.h>
#include <kernel/logging.h>
#include <cpuid.h>
#include <io/ports.h>
#include <stdbool.h>
extern void hcf(void);

uint64_t apic_hh_address;
bool x2mode;

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

uint32_t read_apic_register(uint32_t reg_off) {
    if (x2mode)
        return (uint32_t) rdmsr((reg_off >> 4) + 0x800);
    else
        return *(volatile uint32_t*)(apic_hh_address + reg_off);
}

void write_apic_register(uint32_t reg_off, uint32_t val) {
    if (x2mode)
        wrmsr((reg_off >> 4) + 0x800, val);
    else
        *(volatile uint32_t*)(apic_hh_address + reg_off) = val;
}
