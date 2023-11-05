#include <cpu/rtc.h>
#include <io/stdio.h>
#include <io/ports.h>

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
    unsigned char status;
    status = read_cmos_register(0xB);
    status |= 0x2;
    status |= 0x10;
    status &= ~0x20;
    status &= ~0x40;
    bcd = !(status & 0x4);
    printf("BCD: %d\n", bcd);
    write_cmos_register(0xB, status);
    read_cmos_register(0xC);
}

void rtc_handler(void) {
    printf("%b\n", read_cmos_register(0xC));
    printf("%x %x %x %x %x %x\n",
            read_cmos_register(0),
            read_cmos_register(2),
            read_cmos_register(4),
            read_cmos_register(8),
            read_cmos_register(9),
            read_cmos_register(0x32));
    // flush_screen();
}
