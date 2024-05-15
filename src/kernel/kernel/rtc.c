#include <cpu/cpu.h>
#include <cpu/idt.h>
#include <cpu/lapic.h>
#include <cpu/ioapic.h>
#include <kernel/rtc.h>
#include <kernel/logging.h>
#include <drivers/framebuffer.h>
#include <io/ports.h>
#include <io/stdio.h>

static int bcd;
static time_t global_time;

static uint8_t read_register(uint8_t port_num) {
    outb(0x70, port_num | 0x80);
    return inb(0x71);
}

static void write_register(uint8_t port_num, uint8_t val) {
    outb(0x70, port_num | 0x80);
    outb(0x71, val);
}

static uint32_t bcd2bin(uint32_t bcd_num) {
    return ((bcd_num >> 4) * 10) + (bcd_num & 0xF);
}

// Month is 1-12
// Copied and pasted from Wikipedia article
static uint8_t get_dow(uint16_t y, uint8_t m, uint8_t dom) {
    return (dom +=m < 3 ? y-- : y - 2, 23 * m / 9 + dom + 4 + y / 4 - y / 100 + y / 400) % 7 + 1;
}

void init_rtc(void) {
    IDT_SET_INT(34, 0, rtc_irq);
    set_irq(8, 34, 0xFF, IOAPIC_LOW_PRIORITY, true);
    uint8_t status = read_register(0xB);
    status |= 0x2 | 0x10; // 24 hour mode and update ended interrupt
    status &= ~0x20; // No periodic interrupt and no alarm interrupt
    status &= ~0x40;
    bcd = !(status & 0x4);
    write_register(0xB, status);
    log(Info, "RTC", "Initialized RTC timer (in %s mode)", bcd ? "BCD" : "binary");
    read_register(0xC);
    set_irq_mask(8, false);
}

cpu_status_t *rtc_handler(cpu_status_t *status) {
    uint8_t rtc_status = read_register(0xC);
    if (rtc_status & 0b10000) {
        if (bcd) {
            uint8_t century = bcd2bin(read_register(0x32));
            global_time.century = century != 0 ? century : 20;
            global_time.year = bcd2bin(read_register(0x9));
            global_time.month = bcd2bin(read_register(0x8));
            global_time.dom = bcd2bin(read_register(0x7));
            global_time.hour = bcd2bin(read_register(0x4));
            global_time.minute = bcd2bin(read_register(0x2));
            global_time.second = bcd2bin(read_register(0));
        } else {
            uint8_t century = read_register(0x32);
            global_time.century = century != 0 ? century : 20;
            global_time.year = read_register(0x9);
            global_time.month = read_register(0x8);
            global_time.dom = read_register(0x7);
            global_time.hour = read_register(0x4);
            global_time.minute = read_register(0x2);
            global_time.second = read_register(0);
        }
        global_time.dow = get_dow(global_time.century * 100 + global_time.year, global_time.month, global_time.dom);
        // HH:MM:SS
        // MM/DD/YY
        uint32_t old_color = screen_fg;
        screen_fg = 0x579cf7;
        printf_at(screen_num_cols - TIME_LENGTH, 0, "%s%u:%s%u:%s%u",
                    global_time.hour < 10 ? "0" : "", global_time.hour,
                    global_time.minute < 10 ? "0" : "", global_time.minute,
                    global_time.second < 10 ? "0" : "", global_time.second);
        printf_at(screen_num_cols - DATE_LENGTH, 1, "%s%u/%s%u/%s%u",
                    global_time.month < 10 ? "0" : "", global_time.month,
                    global_time.dom < 10 ? "0" : "", global_time.dom,
                    global_time.year < 10 ? "0" : "", global_time.year);
        screen_fg = old_color;
    }
    APIC_EOI();
    return status;
}