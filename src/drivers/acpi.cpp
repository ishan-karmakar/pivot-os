#include <cpu/cpu.hpp>
#include <lib/logger.hpp>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>
#include <uacpi/event.h>
#include <uacpi/sleep.h>
#include <uacpi/utilities.h>
#include <uacpi/notify.h>
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

void acpi::late_init() {
    assert(uacpi_likely_success(uacpi_namespace_initialize()));
    assert(uacpi_likely_success(uacpi_install_fixed_event_handler(UACPI_FIXED_EVENT_POWER_BUTTON, [](uacpi_handle) -> uacpi_interrupt_ret {
        uacpi_kernel_schedule_work(UACPI_WORK_GPE_EXECUTION, [](uacpi_handle) { acpi::shutdown(); }, nullptr);
        return UACPI_INTERRUPT_HANDLED;
    }, nullptr)));
    assert(uacpi_likely_success(uacpi_find_devices("PNP0C0C", [](void*, uacpi_namespace_node *node) -> uacpi_ns_iteration_decision {
        assert(uacpi_likely_success(uacpi_install_notify_handler(node, [](void*, uacpi_namespace_node*, uint64_t v) -> uacpi_status {
            if (v == 0x80)
                acpi::shutdown();
            return UACPI_STATUS_OK;
        }, nullptr)));
        return UACPI_NS_ITERATION_DECISION_CONTINUE;
    }, nullptr)));
}

// TODO: Move to kernel.cpp and provide full shutdown
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
