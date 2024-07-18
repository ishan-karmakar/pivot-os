#include <drivers/rtc.hpp>
#include <drivers/ioapic.hpp>
#include <cpu/cpu.hpp>
using namespace drivers;
extern "C" void rtc_irq();

bool RTC::bcd;

void RTC::init(cpu::IDT& idt) {
    idt.set_entry(RTC_IDT_ENT, 0, rtc_irq);
    IOAPIC::set_irq(8, RTC_IDT_ENT, 0, IOAPIC::LowestPriority | IOAPIC::Masked);

    // uint8_t status = read_reg(0xB);
    // status |= 0x2 | 0x10;
    // status &= ~(0x20 | 0x40);

    // bcd = !(status & 0x4);
    // write_reg(0xB, status);
    log(Info, "RTC", "Initialized RTC timer in (in %s mode)", bcd ? "BCD" : "binary");
    // read_reg(0xC);
    // IOAPIC::set_mask(8, false);
}

extern "C" cpu::cpu_status *rtc_handler(cpu::cpu_status *status) {
    return status;
}
