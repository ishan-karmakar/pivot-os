#pragma once
#include <stdint.h>

typedef struct {
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
