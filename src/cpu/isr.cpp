#include <cpu/cpu.hpp>
#include <cpu/idt.hpp>
#include <lib/logger.hpp>
// TODO: Seperate stack (IST)
// TODO: Return from recoverable exceptions

void log_registers(cpu::status *status) {
    logger::verbose("ISR", "ERROR code: 0x%lx", status->err_code);
    logger::verbose("ISR", "ss: %p, rsp: %p, rflags: %p, cs: %p",
        status->ss, status->rsp, status->rflags, status->cs);
    logger::verbose("ISR", "rip: %p, rax: %p, rbx: %p, rcx: %p",
        status->rip, status->rax, status->rbx, status->rcx);
    logger::verbose("ISR", "rdx: %p, rbp: %p, rsi: %p, rdi: %p",
        status->rdx, status->rbp, status->rsi, status->rdi);
    logger::verbose("ISR", "r8: %p, r9: %p, r10: %p, r11: %p",
        status->r8, status->r9, status->r10, status->r11);
    logger::verbose("ISR", "r12: %p, r13: %p, r14: %p, r15: %p",
        status->r12, status->r13, status->r14, status->r15);
}

[[noreturn]]
void exception_handler(cpu::status *status) {
    logger::error("ISR", "Received exception %lu", status->int_no);
    switch (status->int_no) {
    case 14:
        uint64_t cr2;
        asm volatile ("mov %%cr2, %0" : "=r" (cr2));
        logger::verbose("ISR", "CR2: %p", cr2);
        log_registers(status);
        break;
    
    default:
        log_registers(status);
    }
    cpu::hcf();
}

extern "C" {
    cpu::status *int_handler(cpu::status *status) {
        if (status->int_no < 32)
            exception_handler(status);

        auto& h = idt::handlers();
        uint8_t irq = status->int_no - 32;
        cpu::status *ret_status = nullptr;
        for (auto handler : h[irq]) {
            auto new_status = handler(status);
            if (new_status)
                ret_status = new_status;
        }
        return ret_status ? ret_status : status;
    }
}
