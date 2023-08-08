#include <stdint.h>
#include <cpu/lapic.h>
#include <kernel/logging.h>
#include <libc/string.h>
#include <drivers/keyboard.h>

extern void hcf(void);

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    
    uint64_t interrupt_number;
    uint64_t error_code;
    
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) cpu_status_t ;

static const char *exception_names[] = {
    "Divide by Zero Error",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

void exception_handler(cpu_status_t *status) {
    log(Error, "ISR", "Got exception %s, %x\n", exception_names[status->interrupt_number], status->error_code);
    if (status->interrupt_number == 8 || status->interrupt_number == 0x12 || status->interrupt_number == 0xE)
        hcf();
}

void irq_handler(uint64_t interrupt_number) {
    switch (interrupt_number) {
        case 33:
            handle_keyboard();
            write_eoi();
            break;
        default:
            log(Info, "IRQ", "IRQ %u triggered", interrupt_number);
    }
}
