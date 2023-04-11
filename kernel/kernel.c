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

    for (;;);
}