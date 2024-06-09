#pragma once
#include <stdint.h>
#define APIC_PERIODIC_IDT_ENTRY 32
#define APIC_ONESHOT_IDT_ENTRY 33
#define PIT_IDT_ENTRY 34
#define RTC_IDT_ENTRY 35
#define KEYBOARD_IDT_ENTRY 36
#define SYSCALL_IDT_ENTRY 0x80
#define IPI_IDT_ENTRY 0x81
#define APIC_SPURIOUS_IDT_ENTRY 255

#define IDT_SET_ENTRY(num, flags, sel, ist, handler) \
    extern void handler(); \
    set_idt_entry((num), (flags), (sel), (ist), (handler));

#define IDT_SET_INT(num, ring, handler) IDT_SET_ENTRY((num), 0x8E | ((ring) << 5), 0x8, 0, (handler))
#define IDT_SET_TRAP(num, ring, handler) IDT_SET_ENTRY((num), 0x8F | ((ring) << 5), 0x8, 0, (handler))

#pragma pack(push, 1)

typedef struct idt_desc {
    uint16_t offset0;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset1;
    uint32_t offset2;
    uint32_t rsv;
} idt_desc_t;

typedef struct idtr {
    uint16_t size;
    uintptr_t addr;
} idtr_t;

#pragma pack(pop)

/// @brief Set entry in IDT
/// @param idx IDT entry number
/// @param flags IDT entry flags
/// @param selector IDT entry code selector
/// @param ist IDT entry IST
/// @param handler IDT entry handler function
void set_idt_entry(uint16_t idx, uint8_t flags, uint16_t selector, uint8_t ist, void (*handler)(void));

/// @brief Load the IDT
void init_idt(void);