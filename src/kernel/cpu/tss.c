#include <cpu/tss.h>
#include <kernel/logging.h>

extern uint64_t stack[];

tss_t tss = { 0 };

void init_tss(void) {
    tss.rsp0 = (uintptr_t) stack + 16384;
}