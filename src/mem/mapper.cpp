#include <mem/mapper.hpp>
#include <kernel.hpp>
#include <util/logger.hpp>
#include <mem/pmm.hpp>
#include <mem/heap.hpp>
#include <uacpi/kernel_api.h>

using namespace mapper;
constexpr uintptr_t SIGN_MASK = 0x000ffffffffff00;

frg::manual_box<PTMapper> mapper::kmapper;

void mapper::init() {
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
    log(VERBOSE, "MAPPER[MAP]", "%p -> %p", virt, phys);
    if (phys == 0 || virt == 0)
        return;

    flags |= 1;
    auto indexes = get_entries(virt);
    uint16_t p4_idx = indexes[0], p3_idx = indexes[1], p2_idx = indexes[2], p1_idx = indexes[3];
    virt = round_down(virt, PAGE_SIZE);
    phys = round_down(phys, PAGE_SIZE);

    if (!(pml4[p4_idx] & 1)) {
        log(DEBUG, "MAPPER[MAP]", "Allocating new PML4 table");
        pml4[p4_idx] = alloc_table() | KERNEL_ENTRY | 1;
    }

    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1)) {
        log(DEBUG, "MAPPER[MAP]", "Allocating new P3 table");
        p3_tbl[p3_idx] = alloc_table() | KERNEL_ENTRY | 1;
    }

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));

    // Will keep hugepage mapping if the target virtual address still points to right phys address
    uintptr_t hp_phys = round_down(phys, HUGEPAGE_SIZE);
    if ((p2_tbl[p2_idx] & SIGN_MASK) == hp_phys) {
        log(DEBUG, "MAPPER[MAP]", "Hugepage mapping is still valid");
        return;
    }

    if (!(p2_tbl[p2_idx] & 1)) {
        log(DEBUG, "MAPPER[MAP]", "Allocating new P2 table");
        p2_tbl[p2_idx] = alloc_table() | KERNEL_ENTRY | 1;
    } else if (p2_tbl[p2_idx] & 0x80) {
        log(DEBUG, "MAPPER[MAP]", "Converting hugepage mapping to P1 table");
        uintptr_t original = p2_tbl[p2_idx];
        original &= ~0x80UL;
        uintptr_t addr = alloc_table();
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(virt_addr(addr));
        for (int i = 0; i < 512; i++)
            table[i] = ((original & SIGN_MASK) + PAGE_SIZE * i) | (original & ~SIGN_MASK);
        p2_tbl[p2_idx] = addr | KERNEL_ENTRY | 1;
    }

    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    p1_tbl[p1_idx] = phys | flags;
}

uintptr_t PTMapper::alloc_table() {
    uintptr_t paddr = pmm::frame();
    uintptr_t vaddr = virt_addr(paddr);
    pg_tbl_t table = reinterpret_cast<pg_tbl_t>(vaddr);
    clean_table(table);
    map(paddr, vaddr, KERNEL_ENTRY);
    return paddr;
}

uintptr_t PTMapper::translate(uintptr_t virt) const {
    auto indexes = get_entries(virt);
    uint16_t p4_idx = indexes[0], p3_idx = indexes[1], p2_idx = indexes[2], p1_idx = indexes[3];
    virt = round_down(virt, PAGE_SIZE);
    if (!(pml4[p4_idx] & 1)) {
        log(DEBUG, "MAPPER[TRANSLATE]", "No PML4 entry");
        return 0;
    }

    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1)) {
        log(DEBUG, "MAPPER[TRANSLATE]", "No P3 table entry");
        return 0;
    }

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));
    if (!(p2_tbl[p2_idx] & 1)) {
        log(DEBUG, "MAPPER[TRANSLATE]", "No P2 table entry");
        return 0;
    } else if (p2_tbl[p2_idx] & 0x80)
        return (p2_tbl[p2_idx] & SIGN_MASK) + virt % HUGEPAGE_SIZE;
    
    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    uintptr_t addr = p1_tbl[p1_idx] & SIGN_MASK;
    log(VERBOSE, "MAPPER[TRANSLATE]", "%p -> %p", virt, addr);
    return addr;
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

frg::array<uint16_t, 4> PTMapper::get_entries(uintptr_t addr) {
    frg::array<uint16_t, 4> entries;
    for (int i = 0, e = 39; i < 4; i++, e -= 9)
        entries[i] = (addr >> e) & 0x1FF;
    return entries;
}

void *uacpi_kernel_map(uintptr_t phys, size_t) {
    uintptr_t virt = virt_addr(phys);
    kmapper->map(phys, virt, KERNEL_ENTRY);
    return reinterpret_cast<void*>(virt);
}

void uacpi_kernel_unmap(void*, size_t) {}
