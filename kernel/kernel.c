#include "bootparam.h"
#include "string.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "log.h"
#include "mem.h"
#define PAGE_BIT_P_PRESENT (1<<0)
#define PAGE_BIT_RW_WRITABLE (1<<1)
#define PAGE_BIT_US_USER (1<<2)
#define PAGE_ADDR_MASK 0x000ffffffffff000
typedef uint64_t page_table_t[512];
extern void load_pml4(page_table_t*);

__attribute__((aligned(4096)))
page_table_t pml4;

bootparam_t* bootp;
uint64_t next_alloc_page;
void *alloc_page() {
    void *page = (void*) next_alloc_page;
    next_alloc_page += 0x1000;
    return page;
}
void map_page(uint64_t);

static void add_entry(uint64_t *entry, int flags) {
    void *table = alloc_page();
    memset(table, 0, 4096);
    uint64_t addr = (uint64_t)(uintptr_t) table;
    *entry = (addr & PAGE_ADDR_MASK) | flags;
}

void map_page(uint64_t virtual_addr) {
    int flags = PAGE_BIT_P_PRESENT | PAGE_BIT_RW_WRITABLE | PAGE_BIT_US_USER;
    int pml4_idx = (virtual_addr >> 39) & 0x1ff;
    int pdp_idx = (virtual_addr >> 30) & 0x1ff;
    int pd_idx = (virtual_addr >> 21) & 0x1ff;
    int pt_idx = (virtual_addr >> 12) & 0x1ff;

    if (!(pml4[pml4_idx] & PAGE_BIT_P_PRESENT))
        add_entry(&(pml4[pml4_idx]), flags);

    page_table_t *pdpt = (page_table_t*)(pml4[pml4_idx] & PAGE_ADDR_MASK);
    if (!(*pdpt[pdp_idx] & PAGE_BIT_P_PRESENT))
        add_entry(&(*pdpt[pdp_idx]), flags);

    page_table_t *pdt = (page_table_t*)(*pdpt[pdp_idx] & PAGE_ADDR_MASK);
    if (!(*pdt[pt_idx] & PAGE_BIT_P_PRESENT))
        add_entry(&(*pdt[pd_idx]), flags);

    page_table_t *pt = (page_table_t*)(*pdt[pd_idx] & PAGE_ADDR_MASK);
    if (!(*pt[pt_idx] & PAGE_BIT_P_PRESENT))
        *pt[pt_idx] = (virtual_addr & PAGE_ADDR_MASK) | flags;
}

void _start(bootparam_t *bootparam)
{
    bootp = bootparam;
    next_alloc_page = bootp->mem_region.physical_start;
    init_screen();
    load_gdt();
    // load_idt();
    print_num(bootp->mem_region.num_pages);
    for (uint64_t i = 0; i < bootp->mem_region.num_pages * 0x1000; i += 0x1000) {
        map_page(i);
    }
    load_pml4(&pml4);
    for (;;);
}