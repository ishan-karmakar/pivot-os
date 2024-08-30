#include <cpu/cpu.hpp>
#include <io/serial.hpp>
#include <drivers/rtc.hpp>
#include <cpu/idt.hpp>
#include <lib/logger.hpp>
#include <lib/interrupts.hpp>
#include <drivers/ioapic.hpp>
#include <drivers/term.hpp>

using namespace rtc;

bool bcd;
void write_reg(uint8_t, uint8_t);
uint8_t read_reg(uint8_t);
uint8_t set_dow(uint8_t, uint8_t, uint8_t);
uint8_t bcd2bin(uint8_t);

void rtc::init() {
    uint8_t status = read_reg(0xB);
    status |= 0x2 | 0x10;
    status &= ~(0x20 | 0x40);

    bcd = !(status & 0x4);
    write_reg(0xB, status);
    logger::info("RTC", "Initialized RTC timer");
    read_reg(0xC);
}

rtc::time_t rtc::now() {
    time_t t;
    uint8_t rtc_status = read_reg(0xC);
    if (rtc_status & 0b10000) {
        uint8_t century = read_reg(0x32);
        t.century = century ? century : (bcd ? 0x20 : 20);
        t.year = read_reg(0x9);
        t.month = read_reg(0x8);
        t.dom = read_reg(0x7);
        t.hour = read_reg(0x4);
        t.minute = read_reg(0x2);
        t.second = read_reg(0);

        if (bcd) {
            auto data = reinterpret_cast<uint8_t*>(&t);
            for (int i = 0; i < 7; i++)
                data[i] = bcd2bin(data[i]);
        }

        t.dow = set_dow(t.year, t.month, t.dom);
    }

    return t;
}

uint8_t read_reg(uint8_t port) {
    io::out<uint8_t>(0x70, port | 0x80);
    return io::in<uint8_t>(0x71);
}

void write_reg(uint8_t port, uint8_t val) {
    io::out<uint8_t>(0x70, port | 0x80);
    io::out<uint8_t>(0x71, val);
}

uint8_t bcd2bin(uint8_t num) {
    return ((num >> 4) * 10) + (num & 0xF);
}

uint8_t set_dow(uint8_t y, uint8_t m, uint8_t dom) {
    return (dom +=m < 3 ? y-- : y - 2, 23 * m / 9 + dom + 4 + y / 4 - y / 100 + y / 400) % 7 + 1;
}
