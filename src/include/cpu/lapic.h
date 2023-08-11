#pragma once
#include <stddef.h>
#include <stdint.h>
#define APIC_BASE_ADDRESS_MASK 0xFFFFF000
#define APIC_SPURIOUS_VEC_REG_OFF 0xF0
#define APIC_TIMER_INITIAL_COUNT_REG_OFF 0x380
#define APIC_TIMER_CURRENT_COUNT_REG_OFF 0x390
#define APIC_TIMER_CONFIG_OFF 0x3E0
#define APIC_GLOBAL_ENABLE_BIT 11
#define APIC_SOFTWARE_ENABLE (1 << 8)
#define APIC_SPURIOUS_INTERRUPT 255
#define APIC_TIMER_DIVIDER_2 0
#define APIC_TIMER_IDT_ENTRY 0x20
#define APIC_TIMER_LVT_OFFSET 0x320

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

#define PIT_CHANNEL_0_DATA_PORT 0x40
#define PIT_MODE_COMMAND_REGISTER 0x43
#define PIT_1_MS 1193

void init_apic(size_t);
void calibrate_apic_timer(void);
void write_apic_register(uint32_t, uint32_t);
void start_apic_timer(uint32_t);