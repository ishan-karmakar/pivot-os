#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>
#include <uacpi/event.h>
#include <uacpi/sleep.h>
#include <uacpi/utilities.h>
#include <cpu/idt.hpp>
#include <drivers/acpi.hpp>
#include <drivers/lapic.hpp>
#include <drivers/madt.hpp>
#include <drivers/ioapic.hpp>
#include <limine.h>
#include <assert.h>
using namespace acpi;

__attribute__((section(".requests")))
static limine_rsdp_request rsdp_request = { LIMINE_RSDP_REQUEST, 2, nullptr };

extern "C" void acpi_irq();

void acpi::init() {
    assert(rsdp_request.response);

    uacpi_init_params init_params = {
        .rsdp = reinterpret_cast<uintptr_t>(rsdp_request.response->address),
        .log_level = (uacpi_log_level) logger::LOG_LEVEL,
        .flags = 0
    };

    assert(uacpi_likely_success(uacpi_initialize(&init_params)));
    logger::info("ACPI", "Initialized uACPI");

    madt = new (class madt){acpi::get_table(ACPI_MADT_SIGNATURE)};

    ioapic::init();
    lapic::start(lapic::ms_ticks);
    assert(uacpi_likely_success(uacpi_namespace_load()));
    assert(uacpi_likely_success(uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC)));
    assert(uacpi_likely_success(uacpi_finalize_gpe_initialization()));
}

void acpi::shutdown() {
    logger::info("UACPI", "Shutting down system");
    asm volatile ("sti");
    assert(uacpi_likely_success(uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_MAX)));
    asm volatile ("cli");
    assert(uacpi_likely_success(uacpi_enter_sleep_state(UACPI_SLEEP_STATE_MAX)));
    __builtin_unreachable();
}

acpi_sdt_hdr *acpi::get_table(const char *sig) {
    uacpi_table t;
    assert(uacpi_likely_success(uacpi_table_find_by_signature(sig, &t)));

    return t.hdr;
}
