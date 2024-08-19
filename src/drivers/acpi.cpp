#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>
#include <cpu/idt.hpp>
#include <drivers/acpi.hpp>
#include <drivers/lapic.hpp>
#include <drivers/madt.hpp>
#include <drivers/ioapic.hpp>
#include <limine.h>
using namespace acpi;

__attribute__((section(".requests")))
static volatile limine_rsdp_request rsdp_request = { LIMINE_RSDP_REQUEST, 2, nullptr };

extern "C" void acpi_irq();

void acpi::init() {
    ASSERT(rsdp_request.response);

    uacpi_init_params init_params = {
        .rsdp = reinterpret_cast<uintptr_t>(rsdp_request.response->address),
        .log_level = (uacpi_log_level) LOG_LEVEL,
        .flags = 0
    };

    ASSERT(uacpi_likely_success(uacpi_initialize(&init_params)));
    logger::info("ACPI[INIT]", "Initialized uACPI");

    madt = new MADT{acpi::get_table(ACPI_MADT_SIGNATURE)};

    ioapic::init();
    lapic::start(lapic::ms_ticks, lapic::Periodic);
    ASSERT(uacpi_likely_success(uacpi_namespace_load()));
}

acpi_sdt_hdr *acpi::get_table(const char *sig) {
    uacpi_table t;
    ASSERT(uacpi_likely_success(uacpi_table_find_by_signature(sig, &t)));

    return t.hdr;
}

SDT::SDT(const acpi_sdt_hdr *header) : header{header} {}