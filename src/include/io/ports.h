#pragma once
#include <stdint.h>

void outportb(int port, uint8_t data);
uint8_t inportb(int port);