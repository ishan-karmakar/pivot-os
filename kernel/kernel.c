#include "bootparam.h"
#include "string.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "log.h"

bootparam_t* bootp;

/**
 * Example "kernel"
 */
void _start(bootparam_t *bootpar)
{
    bootp = bootpar;
    init_screen();
    load_gdt();
    load_idt();

    uint64_t cr3;
    asm volatile (
        "movl %%cr3, %%eax\n"
        "movl %%eax, %0\n"
        : "=m" (cr3) : : "%eax"
    );

    print_num(cr3);

    for (;;);
}