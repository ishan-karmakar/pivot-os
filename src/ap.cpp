#include <lib/logger.hpp>
#include <cpu/smp.hpp>
#include <limine.h>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>

extern "C" [[noreturn]] void ainit(limine_smp_info *info) {
    smp::ap_init(info);
    logger::info("AP[INIT]", "AP %lu started", info->lapic_id);
    cpu::init();
    gdt::load();
    idt::load();
    logger::info("AP[INIT]", "Completed initialization");
    while(1);
}