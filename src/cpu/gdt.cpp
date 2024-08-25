#include <cpu/gdt.hpp>
#include <algorithm>
#include <lib/logger.hpp>
#include <drivers/acpi.hpp>
#include <string.h>
#include <limine.h>
using namespace gdt;

struct [[gnu::packed]] gdtr_t {
    uint16_t size;
    uintptr_t addr;
};

gdt::desc sgdt[3];

gdt::desc *desc_buffer = sgdt;
uint16_t gdt::num_entries;
constinit gdtr_t gdtr;

extern limine_smp_request smp_request;

void gdt::early_init() {
    set(1, 0b10011011, 0b10); // Kernel CS
    set(2, 0b10010011, 0); // Kernel DS
    load();
    logger::info("GDT[EARLY INIT]", "Loaded compile time GDT");
}

void gdt::init() {
    desc *heap_gdt = new desc[5 + smp_request.response->cpu_count * 2]();
    memcpy(heap_gdt, sgdt, sizeof(desc) * 3);
    desc_buffer = heap_gdt;
    set(3, 0b11111011, 0b10);
    set(4, 0b11110011, 0);
    load();

    logger::info("GDT[INIT]", "Switched to heap allocated GDT");
}

void gdt::set(uint16_t idx, uint8_t access, uint8_t flags) {
    gdt::desc desc { { 0xFFFF, 0, 0, access, 0xF, flags, 0 } };
    set(idx, desc);
}

void gdt::set(uint16_t idx, gdt::desc desc) {
    desc_buffer[idx] = desc;
    num_entries = std::max(static_cast<uint16_t>(idx + 1), num_entries);
}

gdt::desc gdt::get(uint16_t idx) { return desc_buffer[idx]; }

void gdt::load() {
    gdtr.size = num_entries * sizeof(desc) - 1;
    gdtr.addr = reinterpret_cast<uintptr_t>(desc_buffer);
    asm volatile (
        "lgdt %0;"
        "push %1;"
        "push $.L%=;"
        "lretq;"
        ".L%=:"
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"
        "mov %%ax, %%ss;"
        : : "rm" (gdtr), "i" (KCODE), "a" (KDATA) : "memory"
    );
};