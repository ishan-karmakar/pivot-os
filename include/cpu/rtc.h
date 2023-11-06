#pragma once
#include <stdint.h>

typedef struct {
    uint8_t century;
    uint8_t year;
    uint8_t month;
    uint8_t dom;
    uint8_t dow;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} time_t;

void init_rtc(void);
