#include <stdint.h>
#include <cpu/cpu.h>
#include <kernel/logging.h>
extern void hcf(void);

void log_registers(cpu_status_t *status) {
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

void exception_handler(cpu_status_t *status) {
    log(Error, "ISR", "Received interrupt %u\n", status->int_no);
    log_registers(status);
    hcf();
}