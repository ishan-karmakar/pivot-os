#include <io/stdio.h>
#include <cpu/lapic.h>
#include <cpu/mp.h>
#include <cpu/idt.h>
#include <sys.h>
#include <kernel/logging.h>
extern idtr_t idtr;

void __attribute__((noreturn)) ap_start(void) {
    log(Info, "AP", "Processor %u started", get_apic_id());
    while (1) asm ("pause");
}