#include "bootparam.h"
#include "string.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "log.h"
#define PAGE_TABLE_ENTRIES 512
#define PAGE_SIZE 4096

bootparam_t* bootp;

static inline void set_cr3(uint64_t cr3) {
    asm volatile ("mov %0, %%cr3" : : "r" (cr3));
}

static inline uint64_t get_cr3(void) {
    uint64_t ret;
    asm volatile ("mov %%cr3, %0" : "=r" (ret));
    return ret;
}

static inline void set_cr0(uint64_t cr0) {
    asm volatile ("mov %0, %%cr0" : : "r" (cr0));
}

static inline uint64_t get_cr0(void) {
    uint64_t ret;
    asm volatile ("mov %%cr0, %0" : "=r" (ret));
    return ret;
}

void init_paging(void) {
    uint64_t* pml4 = (uint64_t*) 0x124000;
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
        pml4[i] = 0;

    set_cr3((uint64_t) pml4);
    set_cr0(get_cr0() | 0x80000000);
}

/**
 * Example "kernel"
 */
void _start(bootparam_t *bootpar)
{
    bootp = bootpar;
    init_screen();
    load_gdt();
    load_idt();
    init_paging();
    // setup_page_tables();
    for (;;);
}