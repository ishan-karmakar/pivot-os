#include "bootparam.h"
#include "string.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"

bootparam_t* bootp;

/**
 * Example "kernel"
 */
void _start(bootparam_t *bootpar)
{
    bootp = bootpar;
    init_screen();
    print_string("Hello World from kernel");
    // load_gdt();
    // load_idt();

    // asm ("int $2");
    for (;;);
}