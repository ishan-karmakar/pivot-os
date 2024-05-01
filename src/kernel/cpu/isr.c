#include <stdint.h>
#include <cpu/idt.h>
#include <cpu/cpu.h>
#include <cpu/lapic.h>
#include <scheduler/thread.h>
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

cpu_status_t *exception_handler(cpu_status_t *status) {
    switch (status->int_no) {
        case 14:
            log(Error, "ISR", "Received interrupt number 14");
            log(Verbose, "ISR", "Error code: %b", status->err_code);
            uint64_t cr2 = 0;
            asm volatile ("mov %%cr2, %0" : "=r" (cr2));
            log(Verbose, "ISR", "cr2: %x", cr2);
            log_registers(status);
            hcf();
            break;
        default:
            log(Error, "ISR", "Received interrupt number %u", status->int_no);
            log_registers(status);
            hcf();
    }
    return status;
}

cpu_status_t *syscall_handler(cpu_status_t *status) {
    uint64_t id = status->rax;
    switch (id) {
    case 1:
        thread_sleep_syscall(status);
        break;
    case 2:
        thread_dead_syscall();
        break;
    }
    return status;
}