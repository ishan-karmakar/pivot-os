#pragma once
#include <stddef.h>
#include <cpu/cpu.h>

extern size_t create_task(void (*)(void), uintptr_t);
void print_task(size_t);