#include <cpu/cpu.hpp>
#include <drivers/ioapic.hpp>
#include <cpu/lapic.hpp>
#include <io/serial.hpp>
#include <drivers/rtc.hpp>

using namespace drivers;
extern "C" void rtc_irq();

bool RTC::bcd;
union RTC::time RTC::time;

void RTC::init(cpu::IDT& idt) {
    idt.set_entry(RTC_IDT_ENT, 0, rtc_irq);
    IOAPIC::set_irq(8, RTC_IDT_ENT, 0, IOAPIC::LowestPriority | IOAPIC::Masked);

    uint8_t status = read_reg(0xB);
    status |= 0x2 | 0x10;
    status &= ~(0x20 | 0x40);

    bcd = !(status & 0x4);
    write_reg(0xB, status);
    log(Info, "RTC", "Initialized RTC timer in (in %s mode)", bcd ? "BCD" : "binary");
    read_reg(0xC);
    IOAPIC::set_mask(8, false);
}

void RTC::get_time() {
    uint8_t rtc_status = RTC::read_reg(0xC);
    if (rtc_status & 0b10000) {
        uint8_t century = RTC::read_reg(0x32);
        RTC::time.century = century ? century : (bcd ? 0x20 : 20);
        RTC::time.year = RTC::read_reg(0x9);
        RTC::time.month = RTC::read_reg(0x8);
        RTC::time.dom = RTC::read_reg(0x7);
        RTC::time.hour = RTC::read_reg(0x4);
        RTC::time.minute = RTC::read_reg(0x2);
        RTC::time.second = RTC::read_reg(0);

        if (bcd)
            for (int i = 0; i < 7; i++)
                RTC::time.data[i] = bcd2bin(RTC::time.data[i]);
        RTC::time.dow = get_dow(RTC::time.year, RTC::time.month, RTC::time.dom);
    }
}

uint8_t RTC::read_reg(uint8_t port) {
    io::outb(0x70, port | 0x80);
    return io::inb(0x71);
}

void RTC::write_reg(uint8_t port, uint8_t val) {
    io::outb(0x70, port | 0x80);
    io::outb(0x71, val);
}

uint8_t RTC::bcd2bin(uint8_t num) {
    return ((num >> 4) * 10) + (num & 0xF);
}

uint8_t RTC::get_dow(uint8_t y, uint8_t m, uint8_t dom) {
    return (dom +=m < 3 ? y-- : y - 2, 23 * m / 9 + dom + 4 + y / 4 - y / 100 + y / 400) % 7 + 1;
}

extern "C" cpu::cpu_status *cpu::rtc_handler(cpu::cpu_status *status) {
    RTC::get_time();
    auto lims = io::cout.get_lims();
    auto old_pos = io::cout.get_pos();
    io::cout.set_pos({ lims.first - 8, 0 });
    printf("%u:%u:%u", RTC::time.hour, RTC::time.minute, RTC::time.second);
    io::cout.set_pos(old_pos);
    LAPIC::eoi();
    return status;
}
