#pragma once
#include <stdint.h>

uint64_t rdmsr(uint32_t);

void wrmsr(uint32_t address, uint64_t value);
