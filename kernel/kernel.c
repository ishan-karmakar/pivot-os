#include "bootparam.h"
#include "string.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "log.h"

bootparam_t* bootp;
uint64_t pml4[512] __attribute__((aligned(4096)));
uint64_t pdpte[512] __attribute__((aligned(4096)));
uint64_t pde[512] __attribute__((aligned(4096)));
uint64_t pte[512] __attribute__((aligned(4096)));

void initialize_page_tables(void) {
    for (int i = 0; i < 512; i++) {
        pml4[i] = 0x2;
        pdpte[i] = 0x2;
        pde[i] = 0x2;
        pte[i] = 0x2;
    }
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
    initialize_page_tables();
    for (;;);
}