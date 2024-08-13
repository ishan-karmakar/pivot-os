#include <cpu/cpu.hpp>
#include <util/logger.hpp>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>
#include <cpu/idt.hpp>
#include <drivers/acpi.hpp>
#include <limine.h>
using namespace acpi;

constexpr int IDT_ENT = 36;

__attribute__((section(".requests")))
static volatile limine_rsdp_request rsdp_request = { LIMINE_RSDP_REQUEST, 2, nullptr };

extern "C" void acpi_irq();

void acpi::init() {
    logger::assert(rsdp_request.response, "ACPI[init]", "Limine failed to respond to RSDP request");

    uacpi_init_params init_params = {
        .rsdp = reinterpret_cast<uintptr_t>(rsdp_request.response->address),
        .log_level = UACPI_LOG_TRACE,
        .flags = 0
    };

    logger::assert(uacpi_likely_success(uacpi_initialize(&init_params)), "ACPI[INIT]", "Failed to initialize uACPI");
    logger::assert(uacpi_likely_success(uacpi_namespace_load()), "ACPI[INIT]", "Failed to load uACPI namespace");
    logger::info("ACPI[INIT]", "Finished ACPI initialization");
}

SDT::SDT(const acpi_sdt_hdr *header) : header{header} {
}

extern "C" cpu::cpu_status *acpi_handler(cpu::cpu_status *status) {
    logger::info("uACPI", "ACPI interrupt triggered");
    return status;
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle *out_handle) {
    logger::info("uACPI", "uACPI requested to install interrupt handler");
    // idt::kidt->set_entry(IDT_ENT, 0, [handler, ctx]() {});
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle) {
    logger::info("uACPI", "uACPI requested to uninstall interrupt handler");
    return UACPI_STATUS_UNIMPLEMENTED;
}