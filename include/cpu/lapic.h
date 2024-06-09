#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <mem/pmm.h>
#include <acpi/acpi.h>
#define IA32_APIC_BASE 0x1B
#define APIC_BASE_ADDRESS_MASK 0xFFFFF000
#define APIC_GLOBAL_ENABLE_BIT (1 << 11)
#define PIC_COMMAND_MASTER 0x20
#define PIC_COMMAND_SLAVE 0xA0
#define PIC_DATA_MASTER 0x21
#define PIC_DATA_SLAVE 0xA1
#define APIC_SOFTWARE_ENABLE (1 << 8)

#define APIC_ID_OFF 0x20
#define APIC_TPR_OFF 0x80
#define APIC_APR_OFF 0x90
#define APIC_PPR_OFF 0xA0
#define APIC_EOI_OFF 0xB0
#define APIC_SPURIOUS_VEC_OFF 0xF0
#define APIC_ISR_OFF 0x100
#define APIC_ICRLO_OFF 0x300
#define APIC_ICRHI_OFF 0x310
#define APIC_TIMER_LVT_OFFSET 0x320
#define APIC_TIMER_INITIAL_COUNT_OFF 0x380
#define APIC_TIMER_CURRENT_COUNT_OFF 0x390
#define APIC_TIMER_CONFIG_OFF 0x3E0

#define ICR_DEST_SHIFT 24
#define ICR_INIT (0b101 << 8)
#define ICR_SEND_PENDING 0x1000
#define ICR_ASSERT (1 << 14)
#define ICR_LEVEL (1 << 15)
#define ICR_STARTUP (0b110 << 8)

#define APIC_TIMER_PERIODIC (1 << 17)
#define APIC_TIMER_TSC (1 << 18)

#define APIC_TIMER_DIVIDER_1 0xB
#define APIC_TIMER_DIVIDER_2 0x0
#define APIC_TIMER_DIVIDER_4 0x1
#define APIC_TIMER_DIVIDER_8 0x2
#define APIC_TIMER_DIVIDER_16 0x3
#define APIC_TIMER_DIVIDER_32 0x8
#define APIC_TIMER_DIVIDER_64 0x9
#define APIC_TIMER_DIVIDER_128 0xA

#define APIC_TIMER_DIVIDER APIC_TIMER_DIVIDER_4

#define PIT_CHANNEL_0_DATA_PORT 0x40
#define PIT_MODE_COMMAND_REGISTER 0x43
#define PIT_1_MS 1193

#define APIC_EOI() write_apic_register(APIC_EOI_OFF, 0);

void init_lapic(void);
void init_lapic_ap(void);
uint64_t read_apic_register(uint32_t);
void write_apic_register(uint32_t, uint64_t);
void calibrate_apic_timer(void);
void start_apic_timer(uint32_t timer_mode, size_t initial_count, uint8_t idt_entry);
void map_lapic(page_table_t);
uint32_t get_apic_id(void);
void delay(size_t);