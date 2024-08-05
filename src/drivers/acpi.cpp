#include <cpu/cpu.hpp>
#include <util/logger.hpp>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>
#include <cpu/idt.hpp>
#include <drivers/acpi.hpp>
using namespace acpi;

constexpr int IDT_ENT = 36;
constexpr int IRQ_ENT = 9;
struct uacpi_handler_info {
    uacpi_interrupt_handler handler;
    uacpi_handle ctx;
};
uacpi_handler_info acpi_info;

extern "C" void acpi_irq();

void acpi::init() {
    uacpi_init_params init_params = {
        // .rsdp = bi->rsdp,
        .log_level = UACPI_LOG_TRACE,
        .flags = 0
    };

    uacpi_status status = uacpi_initialize(&init_params);
    if (uacpi_unlikely_error(status)) {
        log(ERROR, "uACPI", "ERROR initializing uACPI");
        abort();
    }
    uacpi_kernel_log(UACPI_LOG_INFO, "uACPI finished initialization\n");
}

extern "C" cpu::cpu_status *acpi_handler(cpu::cpu_status *status) {
    log(INFO, "uACPI", "ACPI interrupt triggered");
    acpi_info.handler(acpi_info.ctx);
    return status;
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle *out_handle) {
    log(INFO, "uACPI", "uACPI requested to install interrupt handler");
    switch (irq) {
    case IRQ_ENT:
        idt::kidt->set_entry(IDT_ENT, 0, acpi_irq);
        acpi_info.handler = handler;
        acpi_info.ctx = ctx;
        break;
    }
    *out_handle = &acpi_info;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle) {
    log(INFO, "uACPI", "uACPI requested to uninstall interrupt handler");
    return UACPI_STATUS_UNIMPLEMENTED;
}