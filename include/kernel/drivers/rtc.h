#pragma once
#include <stdint.h>

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
#define TIME_LENGTH 8
#define DATE_LENGTH 8

typedef struct time {
    uint8_t century; // 19 or 20
    uint8_t year; // 0-99
    uint8_t month; // 1-12
    uint8_t dom; // 1-31
    uint8_t dow; // Sunday is 1, Saturday is 7
    uint8_t hour; // 0-23 or 1-12
    uint8_t minute; // 0-59
    uint8_t second; // 0-59
} time_t;

void init_rtc(void);