#include <cpu/cpu.hpp>
#include <cpu/idt.hpp>
#include <util/logger.h>
#define SET_ENTRY(idt, idx, handler) \
extern void handler(); \
idt.set_entry(idx, 0, (uintptr_t) handler);

#define ISR_SAVE_CONTEXT() \
asm volatile ( \
    "pushq %rax; pushq %rbx; pushq %rcx; pushq %rdx;" \
    "pushq %rbp; pushq %rsi; pushq %rdi; pushq %r8;" \
    "pushq %r9; pushq %r10; pushq %r11; pushq %r12;" \
    "pushq %r13; pushq %r14; pushq %r15;" \
);

#define ISR_RESTORE_CONTEXT() \
asm volatile ( \
    "pop %r15; pop %r14; pop %r13; pop %r12;" \
    "pop %r11; pop %r10; pop %r9; pop %r8;" \
    "pop %rdi; pop %rsi; pop %rbp; pop %rdx;" \
    "pop %rcx; pop %rbx; pop %rax;" \
);

#define ISR_BASE_CODE(num) \
    asm volatile ("pushq $" #num); \
    ISR_SAVE_CONTEXT(); \
    asm volatile ( \
        "mov %rsp, %rdi;" \
        "call exception_handler;" \
    ); \
    ISR_RESTORE_CONTEXT(); \
    asm volatile ("iretq");

#define ISR_ERR(num) \
[[gnu::naked]] void isr##num() { ISR_BASE_CODE(num); }

#define ISR_NOERR(num) \
[[gnu::naked]] void isr##num() { asm volatile ("pushq $0"); ISR_BASE_CODE(num); }

namespace cpu::isr {
    ISR_NOERR(0);
    ISR_NOERR(1);
    ISR_NOERR(2);
    ISR_NOERR(3);
    ISR_NOERR(4);
    ISR_NOERR(5);
    ISR_NOERR(6);
    ISR_NOERR(7);
    ISR_ERR(8);
    ISR_NOERR(9);
    ISR_ERR(10);
    ISR_ERR(11);
    ISR_ERR(12);
    ISR_ERR(13);
    ISR_ERR(14);
    ISR_NOERR(15);
    ISR_NOERR(16);
    ISR_ERR(17);
    ISR_NOERR(18);
    ISR_NOERR(19);
    ISR_NOERR(20);
    ISR_NOERR(21);
    ISR_NOERR(22);
    ISR_NOERR(23);
    ISR_NOERR(24);
    ISR_NOERR(25);
    ISR_NOERR(26);
    ISR_NOERR(27);
    ISR_NOERR(28);
    ISR_NOERR(29);
    ISR_ERR(30);
    ISR_NOERR(31);

    void log_registers(struct cpu::cpu_status *status) {
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

    [[noreturn]]
    extern "C" void exception_handler(struct cpu::cpu_status *status) {
        switch (status->int_no) {
            case 14: {
                log(Error, "ISR", "Received interrupt number 14");
                log(Verbose, "ISR", "Error code: %b", status->err_code);
                uint64_t cr2 = 0;
                asm volatile ("mov %%cr2, %0" : "=r" (cr2));
                log(Verbose, "ISR", "cr2: %x", cr2);
                log_registers(status);
                cpu::hcf();
            } default: {
                log(Error, "ISR", "Received interrupt number %u", status->int_no);
                log_registers(status);
                cpu::hcf();
            }
        }
    }

    void load_exceptions(cpu::InterruptDescriptorTable& idt) {
        SET_ENTRY(idt, 0, isr0);
        SET_ENTRY(idt, 1, isr1);
        SET_ENTRY(idt, 2, isr2);
        SET_ENTRY(idt, 3, isr3);
        SET_ENTRY(idt, 4, isr4);
        SET_ENTRY(idt, 5, isr5);
        SET_ENTRY(idt, 6, isr6);
        SET_ENTRY(idt, 7, isr7);
        SET_ENTRY(idt, 8, isr8);
        SET_ENTRY(idt, 9, isr9);
        SET_ENTRY(idt, 10, isr10);
        SET_ENTRY(idt, 11, isr11);
        SET_ENTRY(idt, 12, isr12);
        SET_ENTRY(idt, 13, isr13);
        SET_ENTRY(idt, 14, isr14);
        SET_ENTRY(idt, 15, isr15);
        SET_ENTRY(idt, 16, isr16);
        SET_ENTRY(idt, 17, isr17);
        SET_ENTRY(idt, 18, isr18);
        SET_ENTRY(idt, 19, isr19);
        SET_ENTRY(idt, 20, isr20);
        SET_ENTRY(idt, 21, isr21);
        SET_ENTRY(idt, 22, isr22);
        SET_ENTRY(idt, 23, isr23);
        SET_ENTRY(idt, 24, isr24);
        SET_ENTRY(idt, 25, isr25);
        SET_ENTRY(idt, 26, isr26);
        SET_ENTRY(idt, 27, isr27);
        SET_ENTRY(idt, 28, isr28);
        SET_ENTRY(idt, 29, isr29);
        SET_ENTRY(idt, 30, isr30);
        SET_ENTRY(idt, 31, isr31);
    }
}