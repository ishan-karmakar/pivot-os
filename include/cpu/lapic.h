#pragma once
#include <stddef.h>
#include <stdint.h>
#define IA32_APIC_BASE 0x1b
#define APIC_BASE_ADDRESS_MASK 0xFFFFF000
#define APIC_SPURIOUS_VEC_REG_OFF 0xF0
#define APIC_TIMER_INITIAL_COUNT_REG_OFF 0x380
#define APIC_TIMER_CURRENT_COUNT_REG_OFF 0x390
#define APIC_ID_REG_OFF 0x20
#define APIC_TIMER_CONFIG_OFF 0x3E0
#define APIC_ICRLO_OFF 0x300
#define APIC_ICRHI_OFF 0x310
#define APIC_GLOBAL_ENABLE_BIT 11
#define APIC_SOFTWARE_ENABLE (1 << 8)
#define APIC_SPURIOUS_INTERRUPT 255
#define APIC_TIMER_LVT_OFFSET 0x320
#define APIC_TIMER_PERIODIC 0x20000
#define APIC_TIMER_PERIODIC_IDT_ENTRY 32
#define APIC_TIMER_ONESHOT_IDT_ENTRY 33

#define ICR_DEST_SHIFT 24
#define ICR_INIT 0x500
#define ICR_SEND_PENDING 0x1000
#define ICR_ASSERT 0x4000
#define ICR_LEVEL 0x8000
#define ICR_STARTUP 0x600

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

#define APIC_TIMER_DIVIDER 0b1001

extern volatile uint64_t apic_ticks;
extern uint32_t apic_ms_interval;
void init_apic(size_t);
void calibrate_apic_timer(void);
void write_apic_register(uint32_t, uint32_t);
uint32_t read_apic_register(uint32_t);
uint32_t get_apic_id(void);
void apic_startup_ap(uint8_t, uint8_t);
void mdelay(size_t);
void udelay(size_t);
void send_ipi(uint8_t, uint8_t);
void start_apic_timer(uint32_t, size_t, uint8_t);