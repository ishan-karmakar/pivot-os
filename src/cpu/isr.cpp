#include <cpu/cpu.hpp>
#include <cpu/idt.hpp>
#include <util/logger.hpp>
#define EXTERN_ENTRY(num) extern void isr##num();
#define SET_ENTRY(idx) kidt->set_entry(idx, 0, isr##idx);

extern "C" {
    EXTERN_ENTRY(0);
    EXTERN_ENTRY(1);
    EXTERN_ENTRY(2);
    EXTERN_ENTRY(3);
    EXTERN_ENTRY(4);
    EXTERN_ENTRY(5);
    EXTERN_ENTRY(6);
    EXTERN_ENTRY(7);
    EXTERN_ENTRY(8);
    EXTERN_ENTRY(9);
    EXTERN_ENTRY(10);
    EXTERN_ENTRY(11);
    EXTERN_ENTRY(12);
    EXTERN_ENTRY(13);
    EXTERN_ENTRY(14);
    EXTERN_ENTRY(15);
    EXTERN_ENTRY(16);
    EXTERN_ENTRY(17);
    EXTERN_ENTRY(18);
    EXTERN_ENTRY(19);
    EXTERN_ENTRY(20);
    EXTERN_ENTRY(21);
    EXTERN_ENTRY(22);
    EXTERN_ENTRY(23);
    EXTERN_ENTRY(24);
    EXTERN_ENTRY(25);
    EXTERN_ENTRY(26);
    EXTERN_ENTRY(27);
    EXTERN_ENTRY(28);
    EXTERN_ENTRY(29);
    EXTERN_ENTRY(30);
    EXTERN_ENTRY(31);
}

void log_registers(cpu::cpu_status *status) {
    log(VERBOSE, "ISR", "ss: %p, rsp: %p, rflags: %p, cs: %p",
        status->ss, status->rsp, status->rflags, status->cs);
    log(VERBOSE, "ISR", "rip: %p, rax: %p, rbx: %p, rcx: %p",
        status->rip, status->rax, status->rbx, status->rcx);
    log(VERBOSE, "ISR", "rdx: %p, rbp: %p, rsi: %p, rdi: %p",
        status->rdx, status->rbp, status->rsi, status->rdi);
    log(VERBOSE, "ISR", "r8: %p, r9: %p, r10: %p, r11: %p",
        status->r8, status->r9, status->r10, status->r11);
    log(VERBOSE, "ISR", "r12: %p, r13: %p, r14: %p, r15: %p",
        status->r12, status->r13, status->r14, status->r15);
}

extern "C" {
    [[noreturn]]
    void exception_handler(cpu::cpu_status *status) {
        switch (status->int_no) {
            case 14: {
                log(ERROR, "ISR", "Received interrupt number 14");
                log(VERBOSE, "ISR", "ERROR code: 0x%lx", status->err_code);
                uint64_t cr2;
                asm volatile ("mov %%cr2, %0" : "=r" (cr2));
                log(VERBOSE, "ISR", "cr2: %p", cr2);
                log_registers(status);
                cpu::hcf();
            } default: {
                log(ERROR, "ISR", "Received interrupt number %lu", status->int_no);
                log_registers(status);
                cpu::hcf();
            }
        }
    }
}

void cpu::idt::load_exceptions() {
    SET_ENTRY(0);
    SET_ENTRY(1);
    SET_ENTRY(2);
    SET_ENTRY(3);
    SET_ENTRY(4);
    SET_ENTRY(5);
    SET_ENTRY(6);
    SET_ENTRY(7);
    SET_ENTRY(8);
    SET_ENTRY(9);
    SET_ENTRY(10);
    SET_ENTRY(11);
    SET_ENTRY(12);
    SET_ENTRY(13);
    SET_ENTRY(14);
    SET_ENTRY(15);
    SET_ENTRY(16);
    SET_ENTRY(17);
    SET_ENTRY(18);
    SET_ENTRY(19);
    SET_ENTRY(20);
    SET_ENTRY(21);
    SET_ENTRY(22);
    SET_ENTRY(23);
    SET_ENTRY(24);
    SET_ENTRY(25);
    SET_ENTRY(26);
    SET_ENTRY(27);
    SET_ENTRY(28);
    SET_ENTRY(29);
    SET_ENTRY(30);
    SET_ENTRY(31);
}

[[gnu::noinline]]
uintptr_t get_rip() {
    return reinterpret_cast<uintptr_t>(__builtin_return_address(0));
}
