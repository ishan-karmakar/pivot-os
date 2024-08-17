#include <cpu/cpu.hpp>
// #include <drivers/ioapic.hpp>
// #include <drivers/lapic.hpp>
// #include <io/serial.hpp>
#include <drivers/rtc.hpp>
// #include <cpu/idt.hpp>
// #include <lib/logger.hpp>

// using namespace drivers;
// extern "C" void rtc_irq();

// union RTC::time RTC::time;
// bool RTC::bcd;

// void RTC::init() {
//     cpu::kidt->set_entry(IDT_ENT, 0, rtc_irq);
//     IOAPIC::set_irq(IDT_ENT, IRQ_ENT, 0, IOAPIC::LOWEST_PRIORITY | IOAPIC::MASKED);

//     uint8_t status = read_reg(0xB);
//     status |= 0x2 | 0x10;
//     status &= ~(0x20 | 0x40);

//     bcd = !(status & 0x4);
//     write_reg(0xB, status);
//     logger::info("RTC", "Initialized RTC timer in (in %s mode)", bcd ? "BCD" : "binary");
//     read_reg(0xC);
//     IOAPIC::set_mask(8, false);
// }

// void RTC::fetch_time() {
//     uint8_t rtc_status = read_reg(0xC);
//     if (rtc_status & 0b10000) {
//         uint8_t century = read_reg(0x32);
//         time.century = century ? century : (bcd ? 0x20 : 20);
//         time.year = read_reg(0x9);
//         time.month = read_reg(0x8);
//         time.dom = read_reg(0x7);
//         time.hour = read_reg(0x4);
//         time.minute = read_reg(0x2);
//         time.second = read_reg(0);

//         if (bcd)
//             for (int i = 0; i < 7; i++)
//                 time.data[i] = bcd2bin(time.data[i]);
//         set_dow();
//     }
// }

// uint8_t RTC::read_reg(uint8_t port) {
//     io::outb(0x70, port | 0x80);
//     return io::inb(0x71);
// }

// void RTC::write_reg(uint8_t port, uint8_t val) {
//     io::outb(0x70, port | 0x80);
//     io::outb(0x71, val);
// }

// uint8_t RTC::bcd2bin(uint8_t num) {
//     return ((num >> 4) * 10) + (num & 0xF);
// }

// void RTC::set_dow() {
//     uint8_t y = time.year;
//     uint8_t dom = time.dom;
//     uint8_t m = time.month;
//     time.dow = (dom +=m < 3 ? y-- : y - 2, 23 * m / 9 + dom + 4 + y / 4 - y / 100 + y / 400) % 7 + 1;
// }

cpu::status *cpu::rtc_handler(cpu::status *status) {
//     RTC::fetch_time();
//     auto lims = io::cout.get_lims();
//     auto old_pos = io::cout.get_pos();
//     io::cout.set_pos({ lims.first - 8, 0 });
//     auto time = RTC::time;
//     printf("%02hhu:%02hhu:%02hhu", time.hour, time.minute, time.second);
//     io::cout.set_pos({ lims.first - 8, 1 });
//     printf("%02hhu/%02hhu/%02hhu", time.month, time.dom, time.year);
//     io::cout.set_pos(old_pos);
//     LAPIC::eoi();
    return status;
}
