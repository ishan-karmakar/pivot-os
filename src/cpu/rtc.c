#include <cpu/rtc.h>
#include <io/stdio.h>
#include <io/ports.h>
#include <kernel/logging.h>

#define RTC_SECONDS 0
#define RTC_MINUTES 2
#define RTC_HOURS 4
#define RTC_WEEKDAY 6
#define RTC_DOM 7
#define RTC_MONTH 8
#define RTC_YEAR 9
#define RTC_CENTURY 0x32
#define RTC_STATUS_A 0xA
#define RTC_STATUS_B 0xB
#define DISABLE_NMI 0x80

char *months[] = { "January", "February", "March", "April", "May", "June", "July",
                   "August", "September", "October", "November", "December" };

time_t global_time;
int bcd;


uint8_t read_cmos_register(uint8_t port_num) {
    outportb(0x70, port_num);
    return inportb(0x71);
}

void write_cmos_register(uint8_t port_num, uint8_t val) {
    outportb(0x70, port_num);
    outportb(0x71, val);
}

inline uint32_t bcd2bin(uint32_t bcd_num) {
    return ((bcd_num >> 4) * 10) + (bcd_num & 0xF);
}

void init_rtc(void) {
    uint8_t status = read_cmos_register(0xB);
    status |= 0x2 | 0x10; // 24 hour mode and update ended interrupt
    status &= ~20 | ~0x40; // No periodic interrupt and no alarm interrupt
    bcd = !(status & 0x4);
    write_cmos_register(0xB, status);
    read_cmos_register(0xC);
    log(Info, "RTC", "Initialized RTC timer (in %s mode)", bcd ? "BCD" : "binary");
}

void rtc_handler(void) {
    uint8_t status = read_cmos_register(0xC);
    if (status & 0b10000) {
        if (bcd) {
            uint8_t century = bcd2bin(read_cmos_register(0x32));
            global_time.century = century != 0 ? century : 20;
            global_time.year = bcd2bin(read_cmos_register(0x9));
            global_time.month = bcd2bin(read_cmos_register(0x8));
            global_time.dom = bcd2bin(read_cmos_register(0x7));
            global_time.dow = 0;
            global_time.hour = bcd2bin(read_cmos_register(0x4));
            global_time.minute = bcd2bin(read_cmos_register(0x2));
            global_time.second = bcd2bin(read_cmos_register(0));
        } else {
            uint8_t century = read_cmos_register(0x32);
            global_time.century = century != 0 ? century : 20;
            global_time.year = read_cmos_register(0x9);
            global_time.month = read_cmos_register(0x8);
            global_time.dom = read_cmos_register(0x7);
            global_time.dow = 0; // FIX THIS
            global_time.hour = read_cmos_register(0x4);
            global_time.minute = read_cmos_register(0x2);
            global_time.second = read_cmos_register(0);
        }
        log(Verbose, "RTC", "%u:%u:%u, %s %u, %u%u",
            global_time.hour, global_time.minute, global_time.second,
            months[global_time.month - 1], global_time.dom, global_time.century, global_time.year);
    }
}
