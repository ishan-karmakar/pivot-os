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

extern "C" cpu::status *acpi_handler(cpu::status *status) {
    logger::info("uACPI", "ACPI interrupt triggered");
    return status;
}