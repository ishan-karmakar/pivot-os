#include <cpu/smp.hpp>
#include <lib/logger.hpp>
#include <cpu/cpu.hpp>
#include <limine.h>
#include <cpu/idt.hpp>
#include <lib/interrupts.hpp>
#include <drivers/lapic.hpp>
#include <drivers/pit.hpp>
using namespace smp;

__attribute__((section(".requests")))
volatile limine_smp_request smp_request = { LIMINE_SMP_REQUEST, 2, nullptr, 1 };

std::size_t smp::cpu_count;
cpu_t *smp::cpus;
extern "C" void ainit(limine_smp_info*);

void smp::early_init() {
    cpu_count = smp_request.response->cpu_count;
    cpus = new cpu_t[cpu_count]();
    std::size_t bsp = smp_request.response->bsp_lapic_id;
    logger::info("SMP", "Number of CPUs: %lu", cpu_count);
    for (std::size_t i = 0; i < cpu_count; i++) {
        limine_smp_info *info = smp_request.response->cpus[i];
        auto cpu_info = cpus + i;
        info->extra_argument = reinterpret_cast<uintptr_t>(cpu_info);
        cpu_info->id = info->lapic_id;
        if (info->lapic_id == bsp)
            cpu::set_kgs(info->extra_argument);
    }
}

void smp::init() {
    asm volatile ("cli");
    for (std::size_t i = 0; i < smp_request.response->cpu_count; i++) {
        limine_smp_info *info = smp_request.response->cpus[i];
        if (info->lapic_id != smp_request.response->bsp_lapic_id) {
            info->goto_address = ainit;
            while (!cpus[i].ready) asm ("pause");
        }
    }
    asm volatile ("sti");
}

void smp::ap_init(limine_smp_info *cpu) {
    cpu::set_kgs(cpu->extra_argument);
}

cpu_t *smp::this_cpu() {
    if (cpus == nullptr) return nullptr;
    return reinterpret_cast<cpu_t*>(cpu::get_kgs());
}