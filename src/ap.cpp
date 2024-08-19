#include <lib/logger.hpp>
#include <cpu/smp.hpp>
#include <limine.h>
#include <cpu/gdt.hpp>
#include <cpu/idt.hpp>
#include <drivers/lapic.hpp>

extern "C" [[noreturn]] void ainit(limine_smp_info *info) {
    logger::info("AP[INIT]", "AP %lu started", info->lapic_id);
    cpu::init();
    gdt::load();
    idt::load();
    lapic::ap_init();
    smp::ap_init(info);
    while(1);
}