#include "bootparam.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"

bootparam_t* bootp;

void _start(bootparam_t *bootparam)
{
    bootp = bootparam;
    init_screen();
    load_gdt();
    load_idt();
    setup_paging();
    while (1);
}