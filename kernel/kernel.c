#include "bootparam.h"
#include "string.h"
#include "screen.h"
#include "gdt.h"

bootparam_t* bootp;

/**
 * Example "kernel"
 */
void _start(bootparam_t *bootpar)
{
    bootp = bootpar;
    init_screen();
    load_gdt();
    for (;;);
}