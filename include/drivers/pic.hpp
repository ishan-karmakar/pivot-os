#pragma once
#include <cstdint>

namespace pic {
    void init();
    void eoi(uint8_t);
    void mask(uint8_t);
    void unmask(uint8_t);
    void disable();
}