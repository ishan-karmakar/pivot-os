#include <lib/logger.hpp>
#include <cpu/smp.hpp>
#include <limine.h>
#include <cpu/gdt.hpp>
#include <cpu/tss.hpp>
#include <cpu/idt.hpp>
#include <drivers/pit.hpp>
#include <drivers/lapic.hpp>

extern "C" [[noreturn]] void ainit(limine_smp_info *info) {
    cpu::init();
    logger::info("AP", "AP %lu started", info->lapic_id);
    idt::load();
    smp::ap_init(info);
    tss::init();
    lapic::ap_init();
    tss::set_rsp0();
    asm volatile ("sti");
    lapic::start(lapic::ms_ticks);
    smp::this_cpu()->ready = true;
    while(1);
}