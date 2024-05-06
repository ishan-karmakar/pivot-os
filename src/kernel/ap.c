#include <io/stdio.h>
#include <cpu/tss.h>

void ap_kernel(void) {
    init_tss();
    while(1);
}