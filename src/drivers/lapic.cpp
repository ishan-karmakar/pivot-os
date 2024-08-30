#include <drivers/lapic.hpp>
#include <drivers/pic.hpp>
#include <cpu/cpu.hpp>
#include <cpuid.h>
#include <lib/logger.hpp>
#include <drivers/madt.hpp>
#include <cpu/idt.hpp>
#include <mem/mapper.hpp>
#include <mem/pmm.hpp>
#include <lib/interrupts.hpp>
#include <drivers/pit.hpp>
#include <lib/timer.hpp>
#include <cpu/smp.hpp>
#include <uacpi/kernel_api.h>
#include <assert.h>
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

constexpr int SPURIOUS_OFF = 0xF0;
constexpr int ICRLO_OFF = 0x300;
constexpr int ICRHI_OFF = 0x310;
constexpr int INITIAL_COUNT_OFF = 0x380;
constexpr int CUR_COUNT_OFF = 0x390;
constexpr int CONFIG_OFF = 0x3E0;
constexpr int LVT_OFFSET = 0x320;

bool x2mode, tsc;
static uintptr_t addr;
std::size_t lapic::ms_ticks;
std::size_t lapic::ticks;
bool lapic::initialized = false;
uint8_t spurious_vec;

void calibrate();

void lapic::bsp_init() {
    uint64_t msr = cpu::rdmsr(IA32_APIC_BASE);
    assert(msr & (1 << 11));
    
    uint32_t ignored, ecx = 0, edx = 0;
    __cpuid(1, ignored, ignored, ecx, edx);

    if (ecx & (1 << 21))
        x2mode = true;
    else if (edx & (1 << 9)) {
        x2mode = false;
        addr = virt_addr(msr & 0xffffffffff000);
    } else return;

    spurious_vec = idt::set_handler([](cpu::status *status) { return status; });
    timer::irq = intr::IRQ(idt::set_handler([](cpu::status *status) {
        if (smp::this_cpu()->id == smp::bsp_id)
            lapic::ticks++;
        intr::eoi(0);
        return status;
    }));

    write_reg(SPURIOUS_OFF, (1 << 8) | spurious_vec);
    logger::info("LAPIC", "Initialized %sAPIC", x2mode ? "x2" : "x");
    calibrate();
    initialized = true;
}

void lapic::ap_init() {
    write_reg(SPURIOUS_OFF, (1 << 8) | spurious_vec);
}

void calibrate() {
    write_reg(INITIAL_COUNT_OFF, 0);
    write_reg(CONFIG_OFF, TDIV);

    write_reg(INITIAL_COUNT_OFF, (uint32_t) - 1);
    timer::sleep(100);
    uint32_t cur_ticks = read_reg(CUR_COUNT_OFF);

    uint32_t time_elapsed = ((uint32_t) - 1) - cur_ticks;
    ms_ticks = time_elapsed / 100;
    logger::verbose("LAPIC", "APIC ticks per ms: %u", ms_ticks);
}

void lapic::start(std::size_t rate) {
    pit::stop();
    write_reg(LVT_OFFSET, intr::VEC(timer::irq) | (Periodic << 17));
    write_reg(INITIAL_COUNT_OFF, rate);
    write_reg(CONFIG_OFF, TDIV);
}

void lapic::stop() {
    write_reg(INITIAL_COUNT_OFF, 0);
}

void lapic::ipi(uint32_t flags, uint32_t id) {
    if (x2mode)
        write_reg(ICRLO_OFF, (static_cast<uint64_t>(id) << 32) | flags | (1 << 14));
    else {
        write_reg(ICRHI_OFF, id << 24);
        write_reg(ICRLO_OFF, flags | (1 << 14));
    }
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