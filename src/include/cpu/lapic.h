#pragma once
#include <stddef.h>
#define APIC_BASE_ADDRESS_MASK 0xFFFFF000
#define APIC_SPURIOUS_VEC_REG_OFF 0xF0
#define APIC_GLOBAL_ENABLE_BIT 11
#define APIC_SOFTWARE_ENABLE (1 << 8)
#define APIC_SPURIOUS_INTERRUPT 255

#define PIC_COMMAND_MASTER 0x20
#define PIC_COMMAND_SLAVE 0xA0
#define PIC_DATA_MASTER 0x21
#define PIC_DATA_SLAVE 0xA1
#define ICW_1 0x11
#define ICW_2_M 0x20
#define ICW_2_S 0x28
#define ICW_3_M 0x04
#define ICW_3_S 0x02
#define ICW_4 0x01

void init_apic(size_t);
void write_eoi(void);