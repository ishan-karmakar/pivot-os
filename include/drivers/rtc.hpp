#pragma once

namespace rtc {
    struct [[gnu::packed]] time_t {
        uint8_t century;
        uint8_t year;
        uint8_t month;
        uint8_t dom;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint8_t dow;
    };

    time_t now();

    void early_init();
    void init();
}