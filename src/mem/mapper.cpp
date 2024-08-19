#include <kernel.hpp>
#include <lib/logger.hpp>
#include <mem/pmm.hpp>
#include <uacpi/kernel_api.h>
#include <mem/mapper.hpp>
#include <frg/tuple.hpp>
#include <cpu/cpu.hpp>

using namespace mapper;

constexpr uintptr_t SIGN_MASK = 0x000ffffffffff00;

frg::manual_box<ptmapper> mapper::kmapper;

std::array<uint16_t, 4> get_entries(const uintptr_t&);

void mapper::init() {
    uintptr_t cr3 = virt_addr(rdreg(cr3));
    kmapper.initialize(reinterpret_cast<page_table>(cr3));
    logger::debug("MAPPER[INIT]", "PML4: %p", cr3);
    logger::info("MAPPER[INIT]", "Finished initialization");
}

ptmapper::ptmapper(page_table pml4) : pml4{pml4} {}

void ptmapper::map(const uintptr_t& phys, const uintptr_t& virt, const std::size_t& flags, const std::size_t& pages) {
    for (std::size_t i = 0; i < pages; i++)
        map(phys + i * PAGE_SIZE, virt + i * PAGE_SIZE, flags);
}

void ptmapper::map(const uintptr_t& phys, const uintptr_t& virt, const std::size_t& flags) {
    logger::debug("MAPPER[MAP]", "%p -> %p", virt, phys);
    if (phys == 0 || virt == 0)
        return;

    const auto& [p4_idx, p3_idx, p2_idx, p1_idx] = get_entries(round_down(virt, PAGE_SIZE));

    if (!(pml4[p4_idx] & 1)) {
        logger::debug("MAPPER[MAP]", "Allocating new PML4 table");
        pml4[p4_idx] = alloc_table() | KERNEL_ENTRY | 1;
    }

    page_table p3_tbl = reinterpret_cast<page_table>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1)) {
        logger::debug("MAPPER[MAP]", "Allocating new P3 table");
        p3_tbl[p3_idx] = alloc_table() | KERNEL_ENTRY | 1;
    }

    page_table p2_tbl = reinterpret_cast<page_table>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));

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
        page_table table = reinterpret_cast<page_table>(virt_addr(addr));
        for (int i = 0; i < 512; i++)
            table[i] = ((original & SIGN_MASK) + PAGE_SIZE * i) | (original & ~SIGN_MASK);
        p2_tbl[p2_idx] = addr | KERNEL_ENTRY | 1;
    }

    page_table p1_tbl = reinterpret_cast<page_table>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    p1_tbl[p1_idx] = round_down(phys, PAGE_SIZE) | flags | 1;
}

uintptr_t ptmapper::alloc_table() {
    uintptr_t paddr = pmm::frame();
    uintptr_t vaddr = virt_addr(paddr);
    page_table table = reinterpret_cast<page_table>(vaddr);
    clean_table(table);
    map(paddr, vaddr, KERNEL_ENTRY);
    return paddr;
}

uintptr_t ptmapper::translate(const uintptr_t& virt) const {
    const auto& [p4_idx, p3_idx, p2_idx, p1_idx] = get_entries(round_down(virt, PAGE_SIZE));
    if (!(pml4[p4_idx] & 1)) {
        logger::debug("MAPPER[TRANSLATE]", "No PML4 entry");
        return 0;
    }

    page_table p3_tbl = reinterpret_cast<page_table>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1)) {
        logger::debug("MAPPER[TRANSLATE]", "No P3 table entry");
        return 0;
    }

    page_table p2_tbl = reinterpret_cast<page_table>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));
    if (!(p2_tbl[p2_idx] & 1)) {
        logger::debug("MAPPER[TRANSLATE]", "No P2 table entry");
        return 0;
    } else if (p2_tbl[p2_idx] & 0x80)
        return (p2_tbl[p2_idx] & SIGN_MASK) + virt % HUGEPAGE_SIZE;
    
    page_table p1_tbl = reinterpret_cast<page_table>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    uintptr_t addr = p1_tbl[p1_idx] & SIGN_MASK;
    logger::debug("MAPPER[TRANSLATE]", "%p -> %p", virt, addr);
    return addr;
}

void ptmapper::unmap(const uintptr_t& addr, const std::size_t& num_pages) {
    for (std::size_t i = 0; i < num_pages; i++)
        unmap(addr + i * PAGE_SIZE);
}

void ptmapper::unmap(const uintptr_t& virt) {
    logger::debug("MAPPER[UNMAP]", "%p", virt);
    const auto& [p4_idx, p3_idx, p2_idx, p1_idx] = get_entries(round_down(virt, PAGE_SIZE));

    if (!(pml4[p4_idx] & 1))
        return logger::debug("MAPPER[UNMAP]", "No PML4 entry");

    page_table p3_tbl = reinterpret_cast<page_table>(virt_addr(pml4[p4_idx] & SIGN_MASK));
    if (!(p3_tbl[p3_idx] & 1))
        logger::debug("MAPPER[UNMAP]", "No P3 table entry");

    page_table p2_tbl = reinterpret_cast<page_table>(virt_addr(p3_tbl[p3_idx] & SIGN_MASK));
    if (!(p2_tbl[p2_idx] & 1))
        return logger::debug("MAPPER[UNMAP]", "No P2 table entry");
    else if (p2_tbl[p2_idx] & 0x80)
        return logger::debug("MAPPER[UNMAP]", "Found hugepage entry");
    
    page_table p1_tbl = reinterpret_cast<page_table>(virt_addr(p2_tbl[p2_idx] & SIGN_MASK));
    p1_tbl[p1_idx] &= ~1UL;
}

void ptmapper::load() const {
    //
}

void ptmapper::clean_table(page_table tbl) {
    for (int i = 0; i < 512; i++)
        tbl[i] = 0;
}

std::array<uint16_t, 4> get_entries(const uintptr_t& addr) {
    std::array<uint16_t, 4> entries;
    for (int i = 0, e = 39; i < 4; i++, e -= 9)
        entries[i] = (addr >> e) & 0x1FF;
    return entries;
}
