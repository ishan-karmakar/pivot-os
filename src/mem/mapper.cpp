#include <mem/mapper.hpp>
#include <kernel.hpp>
#include <util/logger.hpp>
#include <mem/pmm.hpp>
#include <mem/heap.hpp>
#include <uacpi/kernel_api.h>
#define P_ENTRY(addr, offset) (((addr) >> (offset)) & 0x1FF)

using namespace mem;
constexpr uintptr_t SIGN_MASK = 0x000ffffffffff00;

frg::manual_box<PTMapper> mem::kmapper;

void mem::mapper::init() {
    uintptr_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r" (cr3));
    cr3 = virt_addr(cr3);
    kmapper.initialize(reinterpret_cast<pg_tbl_t>(cr3));
    log(INFO, "MAPPER", "Initialized kernel PT mapper (%p)", cr3);
}

PTMapper::PTMapper(pg_tbl_t pml4) : pml4{pml4} {}

void PTMapper::map(uintptr_t phys, uintptr_t virt, size_t flags, size_t pages) {
    for (size_t i = 0; i < pages; i++)
        map(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, flags);
}

void PTMapper::map(uintptr_t phys, uintptr_t virt, size_t flags) {
    if (phys == 0 || virt == 0)
        return;

    uint16_t p4_idx = P_ENTRY(virt, 39);
    uint16_t p3_idx = P_ENTRY(virt, 30);
    uint16_t p2_idx = P_ENTRY(virt, 21);
    uint16_t p1_idx = P_ENTRY(virt, 12);

    if (!(pml4[p4_idx] & 1)) {
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(virt_addr(pmm::frame()));
        uintptr_t tbl_addr = reinterpret_cast<uintptr_t>(table);
        pml4[p4_idx] = tbl_addr | USER_PT_ENTRY;
        clean_table(table);
        map(tbl_addr, tbl_addr, KERNEL_PT_ENTRY);
    }

    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1)) {
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(virt_addr(pmm::frame()));
        uintptr_t tbl_addr = phys_addr(reinterpret_cast<uintptr_t>(table));
        p3_tbl[p3_idx] = tbl_addr | USER_PT_ENTRY;
        clean_table(table);
        map(tbl_addr, tbl_addr, KERNEL_PT_ENTRY);
    }

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));
    if (flags & 0x80) {
        phys = DIV_FLOOR(phys, HUGEPAGE_SIZE);
        p2_tbl[p2_idx] = phys | flags | 1;
        return;
    }

    if (!(p2_tbl[p2_idx] & 1)) {
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(virt_addr(pmm::frame()));
        uintptr_t tbl_addr = reinterpret_cast<uintptr_t>(table);
        p2_tbl[p2_idx] = tbl_addr | USER_PT_ENTRY;
        clean_table(table);
        map(tbl_addr, tbl_addr, KERNEL_PT_ENTRY);
    } else if (p2_tbl[p2_idx] & 0x80) {
        p2_tbl[p2_idx] &= ~0x80UL;
        map(p2_tbl[p2_idx] & SIGN_MASK, DIV_FLOOR(virt, HUGEPAGE_SIZE), flags, HUGEPAGE_SIZE / PAGE_SIZE);
    }

    phys = DIV_FLOOR(phys, PAGE_SIZE);
    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    p1_tbl[p1_idx] = phys | flags | 1;
}

uintptr_t PTMapper::translate(uintptr_t virt) const {
    virt = DIV_FLOOR(virt, PAGE_SIZE);
    // uint16_t p4_idx = P4_ENTRY(virt);
    // uint16_t p3_idx = P3_ENTRY(virt);
    // uint16_t p2_idx = P2_ENTRY(virt);

    // if (!(pml4[p4_idx] & 1)) return 0;
    
    // pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(pml4[p4_idx] & SIGN_MASK);
    // if (!(p3_tbl[p3_idx] & 1)) return 0;

    // pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(p3_tbl[p3_idx] & SIGN_MASK);
    // if (!(p2_tbl[p2_idx] & 1)) return 0;

    // pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(p2_tbl[p2_idx] & SIGN_MASK);
    // return p1_tbl[p1_idx] & SIGN_MASK;
    return 0;
}

void PTMapper::unmap(uintptr_t, size_t) const {}

void PTMapper::unmap(uintptr_t) const {}

void PTMapper::load() const {
    //
}

void PTMapper::clean_table(pg_tbl_t tbl) {
    for (int i = 0; i < 512; i++)
        tbl[i] = 0;
}

void *uacpi_kernel_map(uintptr_t phys, size_t) {
    // I'm assuming all these addresses will be in physical memory, which is already mapped
    return reinterpret_cast<void*>(phys);
}

void uacpi_kernel_unmap(void*, size_t) {}
