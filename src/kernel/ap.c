#include <io/stdio.h>
#include <cpu/lapic.h>
#include <cpu/mp.h>
#include <cpu/idt.h>
#include <sys.h>
#include <kernel/logging.h>
extern idtr_t idtr;

void __attribute__((noreturn)) ap_start(uintptr_t x) {
    printf("Processor %u has started\n", bsp_id());
    while (1);
}