#include <kernel.hpp>
#include <lib/logger.hpp>
#include <mem/pmm.hpp>
#include <uacpi/kernel_api.h>
#include <mem/mapper.hpp>
#include <frg/tuple.hpp>

using namespace mapper;

constexpr uintptr_t SIGN_MASK = 0x000ffffffffff00;

frg::manual_box<PTMapper> mapper::kmapper;

void mapper::init() {
    uintptr_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r" (cr3));
    cr3 = virt_addr(cr3);
    kmapper.initialize(reinterpret_cast<pg_tbl_t>(cr3));
    logger::debug("MAPPER[INIT]", "PML4: %p", cr3);
    logger::info("MAPPER[INIT]", "Finished initialization");
}

PTMapper::PTMapper(pg_tbl_t pml4) : pml4{pml4} {}

void PTMapper::map(const uintptr_t& phys, const uintptr_t& virt, const size_t& flags, const size_t& pages) {
    for (size_t i = 0; i < pages; i++)
        map(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, flags);
}

void PTMapper::map(const uintptr_t& phys, const uintptr_t& virt, const size_t& flags) {
    logger::debug("MAPPER[MAP]", "%p -> %p", virt, phys);
    if (phys == 0 || virt == 0)
        return;

    const auto& [p4_idx, p3_idx, p2_idx, p1_idx] = get_entries(round_down(virt, PAGE_SIZE));

    if (!(pml4[p4_idx] & 1)) {
        logger::debug("MAPPER[MAP]", "Allocating new PML4 table");
        pml4[p4_idx] = alloc_table() | KERNEL_ENTRY | 1;
    }

    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1)) {
        logger::debug("MAPPER[MAP]", "Allocating new P3 table");
        p3_tbl[p3_idx] = alloc_table() | KERNEL_ENTRY | 1;
    }

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));

    // Will keep hugepage mapping if the target virtual address still points to right phys address
    uintptr_t hp_phys = round_down(phys, HUGEPAGE_SIZE);
    if ((p2_tbl[p2_idx] & SIGN_MASK) == hp_phys) {
        logger::debug("MAPPER[MAP]", "Hugepage mapping is still valid");
        return;
    }

    if (!(p2_tbl[p2_idx] & 1)) {
        logger::debug("MAPPER[MAP]", "Allocating new P2 table");
        p2_tbl[p2_idx] = alloc_table() | KERNEL_ENTRY | 1;
    } else if (p2_tbl[p2_idx] & 0x80) {
        logger::debug("MAPPER[MAP]", "Converting hugepage mapping to P1 table");
        uintptr_t original = p2_tbl[p2_idx];
        original &= ~0x80UL;
        uintptr_t addr = alloc_table();
        pg_tbl_t table = reinterpret_cast<pg_tbl_t>(virt_addr(addr));
        for (int i = 0; i < 512; i++)
            table[i] = ((original & SIGN_MASK) + PAGE_SIZE * i) | (original & ~SIGN_MASK);
        p2_tbl[p2_idx] = addr | KERNEL_ENTRY | 1;
    }

    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    p1_tbl[p1_idx] = round_down(phys, PAGE_SIZE) | flags | 1;
}

uintptr_t PTMapper::alloc_table() {
    uintptr_t paddr = pmm::frame();
    uintptr_t vaddr = virt_addr(paddr);
    pg_tbl_t table = reinterpret_cast<pg_tbl_t>(vaddr);
    clean_table(table);
    map(paddr, vaddr, KERNEL_ENTRY);
    return paddr;
}

uintptr_t PTMapper::translate(const uintptr_t& virt) const {
    const auto& [p4_idx, p3_idx, p2_idx, p1_idx] = get_entries(round_down(virt, PAGE_SIZE));
    if (!(pml4[p4_idx] & 1)) {
        logger::debug("MAPPER[TRANSLATE]", "No PML4 entry");
        return 0;
    }

    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1)) {
        logger::debug("MAPPER[TRANSLATE]", "No P3 table entry");
        return 0;
    }

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));
    if (!(p2_tbl[p2_idx] & 1)) {
        logger::debug("MAPPER[TRANSLATE]", "No P2 table entry");
        return 0;
    } else if (p2_tbl[p2_idx] & 0x80)
        return (p2_tbl[p2_idx] & SIGN_MASK) + virt % HUGEPAGE_SIZE;
    
    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    uintptr_t addr = p1_tbl[p1_idx] & SIGN_MASK;
    logger::debug("MAPPER[TRANSLATE]", "%p -> %p", virt, addr);
    return addr;
}

void PTMapper::unmap(const uintptr_t& addr, const size_t& num_pages) {
    for (size_t i = 0; i < num_pages; i++)
        unmap(addr + i * PAGE_SIZE);
}

void PTMapper::unmap(const uintptr_t& virt) {
    logger::debug("MAPPER[UNMAP]", "%p", virt);
    const auto& [p4_idx, p3_idx, p2_idx, p1_idx] = get_entries(round_down(virt, PAGE_SIZE));

    if (!(pml4[p4_idx] & 1))
        return logger::debug("MAPPER[UNMAP]", "No PML4 entry");

    pg_tbl_t p3_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1))
        logger::debug("MAPPER[UNMAP]", "No P3 table entry");

    pg_tbl_t p2_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));
    if (!(p2_tbl[p2_idx] & 1))
        return logger::debug("MAPPER[UNMAP]", "No P2 table entry");
    else if (p2_tbl[p2_idx] & 0x80)
        return logger::debug("MAPPER[UNMAP]", "Found hugepage entry");
    
    pg_tbl_t p1_tbl = reinterpret_cast<pg_tbl_t>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    p1_tbl[p1_idx] &= ~1UL;
}

void PTMapper::load() const {
    //
}

void PTMapper::clean_table(pg_tbl_t tbl) {
    for (int i = 0; i < 512; i++)
        tbl[i] = 0;
}

std::array<uint16_t, 4> PTMapper::get_entries(const uintptr_t& addr) {
    std::array<uint16_t, 4> entries;
    for (int i = 0, e = 39; i < 4; i++, e -= 9)
        entries[i] = (addr >> e) & 0x1FF;
    return entries;
}

void *uacpi_kernel_map(uintptr_t phys, size_t) {
    uintptr_t virt = virt_addr(phys);
    kmapper->map(phys_addr(phys), virt, KERNEL_ENTRY);
    return reinterpret_cast<void*>(virt);
}

void uacpi_kernel_unmap(void *addr, size_t size) {
    kmapper->unmap(reinterpret_cast<uintptr_t>(virt_addr(addr)), div_ceil(size, PAGE_SIZE));
}
