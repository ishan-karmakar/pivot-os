#include <cpu/cpu.hpp>
#include <util/logger.h>
#include <uacpi/kernel_api.h>
#include <cpu/idt.hpp>
constexpr int IDT_ENT = 36;
constexpr int IRQ_ENT = 9;
struct uacpi_handler_info {
    uacpi_interrupt_handler handler;
    uacpi_handle ctx;
};
uacpi_handler_info acpi_info;

extern "C" void acpi_irq();

extern "C" cpu::cpu_status *acpi_handler(cpu::cpu_status *status) {
    log(Info, "uACPI", "ACPI interrupt triggered");
    acpi_info.handler(acpi_info.ctx);
    return status;
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle *out_handle) {
    log(Info, "uACPI", "uACPI requested to install interrupt handler");
    switch (irq) {
    case IRQ_ENT:
        cpu::kidt->set_entry(IDT_ENT, 0, acpi_irq);
        acpi_info.handler = handler;
        acpi_info.ctx = ctx;
        break;
    }
    *out_handle = &acpi_info;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle) {
    log(Info, "uACPI", "uACPI requested to uninstall interrupt handler");
    return UACPI_STATUS_UNIMPLEMENTED;
}