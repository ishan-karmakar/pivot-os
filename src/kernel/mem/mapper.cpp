#include <mem/mapper.hpp>
#include <common.h>
#include <util/logger.h>
using namespace mem;

PTMapper::PTMapper(pg_tbl_t pml4, PhysicalMemoryManager& pmm) : pml4{pml4}, pmm{pmm} {}

void PTMapper::map(uintptr_t phys, uintptr_t virt, size_t flags, size_t pages) {
    for (size_t i = 0; i < pages; i++)
        map(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, flags);
}

void PTMapper::map(uintptr_t phys, uintptr_t virt, size_t flags) {
    if (phys == 0 || virt == 0)
        return;
    phys = ALIGN_ADDR(phys);
    virt = ALIGN_ADDR(virt);

    uint16_t p4_idx = P4_ENTRY(virt);
    uint16_t p3_idx = P3_ENTRY(virt);
    uint16_t p2_idx = P2_ENTRY(virt);
    uint16_t p1_idx = P1_ENTRY(virt);

    if (!(pml4[p4_idx] & 1)) {
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(pmm.frame());
        uintptr_t tbl_addr = reinterpret_cast<uintptr_t>(table);
        pml4[p4_idx] = tbl_addr | USER_PT_ENTRY;
        clean_table(table);
        map(tbl_addr, tbl_addr, KERNEL_PT_ENTRY);
    }

    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(pml4[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) {
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(pmm.frame());
        uintptr_t tbl_addr = reinterpret_cast<uintptr_t>(table);
        p3_tbl[p3_idx] = tbl_addr | USER_PT_ENTRY;
        clean_table(table);
        map(tbl_addr, tbl_addr, KERNEL_PT_ENTRY);
    }

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) {
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(pmm.frame());
        uintptr_t tbl_addr = reinterpret_cast<uintptr_t>(table);
        p2_tbl[p2_idx] = tbl_addr | USER_PT_ENTRY;
        clean_table(table);
        map(tbl_addr, tbl_addr, KERNEL_PT_ENTRY);
    }

    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(p2_tbl[p2_idx] & SIGN_MASK);
    p1_tbl[p1_idx] = phys | flags;
}

uintptr_t PTMapper::translate(uintptr_t virt) {
    virt = ALIGN_ADDR(virt);
    uint16_t p4_idx = P4_ENTRY(virt);
    uint16_t p3_idx = P3_ENTRY(virt);
    uint16_t p2_idx = P2_ENTRY(virt);
    uint16_t p1_idx = P1_ENTRY(virt);

    if (!(pml4[p4_idx] & 1)) return 0;
    
    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(pml4[p4_idx] & SIGN_MASK);
    if (!(p3_tbl[p3_idx] & 1)) return 0;

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(p3_tbl[p3_idx] & SIGN_MASK);
    if (!(p2_tbl[p2_idx] & 1)) return 0;

    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(p2_tbl[p2_idx] & SIGN_MASK);
    return p1_tbl[p1_idx] & SIGN_MASK;
}

void PTMapper::unmap(uintptr_t, size_t) {}

void PTMapper::unmap(uintptr_t) {}

void PTMapper::load() {
    //
}

void PTMapper::clean_table(pg_tbl_t tbl) {
    for (int i = 0; i < 512; i++)
        tbl[i] = 0;
}
