#pragma once
#include <stdint.h>
#include <stddef.h>
#define IA32_APIC_BASE 0x1B
#define APIC_BASE_ADDRESS_MASK 0xFFFFF000
#define APIC_GLOBAL_ENABLE_BIT 11
#define PIC_COMMAND_MASTER 0x20
#define PIC_COMMAND_SLAVE 0xA0
#define PIC_DATA_MASTER 0x21
#define PIC_DATA_SLAVE 0xA1
#define APIC_SOFTWARE_ENABLE (1 << 8)
#define APIC_SPURIOUS_INTERRUPT 255
#define APIC_SPURIOUS_VEC_REG_OFF 0xF0
#define APIC_TIMER_INITIAL_COUNT_REG_OFF 0x380
#define APIC_TIMER_CURRENT_COUNT_REG_OFF 0x390
#define APIC_TIMER_CONFIG_OFF 0x3E0
#define APIC_TIMER_LVT_OFFSET 0x320

#define APIC_TIMER_PERIODIC 0x20000

#define APIC_TIMER_DIVIDER_1 0xB
#define APIC_TIMER_DIVIDER_2 0x0
#define APIC_TIMER_DIVIDER_4 0x1
#define APIC_TIMER_DIVIDER_8 0x2
#define APIC_TIMER_DIVIDER_16 0x3
#define APIC_TIMER_DIVIDER_32 0x8
#define APIC_TIMER_DIVIDER_64 0x9
#define APIC_TIMER_DIVIDER_128 0xA

#define APIC_TIMER_DIVIDER APIC_TIMER_DIVIDER_8

#define PIT_CHANNEL_0_DATA_PORT 0x40
#define PIT_MODE_COMMAND_REGISTER 0x43
#define PIT_1_MS 1193

#define APIC_EOI_REG_OFF 0xB0

#define APIC_EOI() write_apic_register(APIC_EOI_REG_OFF, 0);

extern volatile size_t pit_ticks, apic_ticks;
extern uint32_t apic_ms_interval;

void init_lapic(void);
uint32_t read_apic_register(uint32_t);
void write_apic_register(uint32_t, uint32_t);
void calibrate_apic_timer(void);
void start_apic_timer(uint32_t timer_mode, size_t initial_count, uint8_t idt_entry);