#include <cpu/smp.hpp>
#include <lib/logger.hpp>
#include <cpu/cpu.hpp>
#include <limine.h>
using namespace smp;

__attribute__((section(".requests")))
volatile limine_smp_request smp_request = { LIMINE_SMP_REQUEST, 2, nullptr, 1 };

cpu_t *cpus;
extern "C" void ainit(limine_smp_info*);

void smp::init() {
    std::size_t cpu_count = smp_request.response->cpu_count;
    cpus = new cpu_t[cpu_count]();
    std::size_t bsp = smp_request.response->bsp_lapic_id;
    logger::info("SMP[INIT]", "Number of CPUs: %lu", cpu_count);
    for (std::size_t i = 0; i < cpu_count; i++) {
        limine_smp_info *info = smp_request.response->cpus[i];
        auto cpu_info = cpus + i;
        info->extra_argument = reinterpret_cast<uintptr_t>(cpu_info);
        cpu_info->id = info->lapic_id;
        if (info->lapic_id != bsp)
            info->goto_address = ainit;
        else
            cpu::set_kgs(info->extra_argument);
    }
    for (std::size_t i = 0; i < cpu_count; i++)
        if (smp_request.response->cpus[i]->lapic_id != bsp)
            while (!cpus[i].ready) asm ("pause");
}

void smp::ap_init(limine_smp_info *cpu) {
    auto cpu_info = reinterpret_cast<cpu_t*>(cpu->extra_argument);
    cpu_info->ready = true;
    cpu::set_kgs(cpu->extra_argument);
}