#include <stdint.h>
#include <kernel/logging.h>
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
} __attribute__((packed)) isr_status_t;

void log_registers(isr_status_t *status) {
    log(Verbose, "ISR", "ss: %x, rsp: %x, rflags: %x, cs: %x",
        status->ss, status->rsp, status->rflags, status->cs);
    log(Verbose, "ISR", "rip: %x, rax: %x, rbx: %x, rcx: %x",
        status->rip, status->rax, status->rbx, status->rcx);
    log(Verbose, "ISR", "rdx: %x, rbp: %x, rsi: %x, rdi: %x",
        status->rdx, status->rbp, status->rsi, status->rdi);
    log(Verbose, "ISR", "r8: %x, r9: %x, r10: %x, r11: %x",
        status->r8, status->r9, status->r10, status->r11);
    log(Verbose, "ISR", "r12: %x, r13: %x, r14: %x, r15: %x",
        status->r12, status->r13, status->r14, status->r15);
}

void exception_handler(isr_status_t *status) {
    log(Error, "ISR", "Received interrupt %u\n", status->interrupt_number);
    log_registers(status);
    hcf();
}