#pragma once
#include <stdint.h>
#define IA32_APIC_BASE 0x1b

uint64_t rdmsr(uint32_t);
void wrmsr(uint32_t address, uint64_t value);

uint8_t inportb(int);
void outportb(int, uint8_t);