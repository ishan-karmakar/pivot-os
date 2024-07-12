#include <boot.h>
#include <drivers/qemu.hpp>
#include <io/stdio.h>
#include <io/stdio.hpp>
#include <util/logger.h>

uint8_t CPU = 0;

extern "C" void __cxa_pure_virtual() { while(1); }

extern "C" void __attribute__((noreturn)) init_kernel(boot_info_t *bi) {
    char_printer = io_char_printer;
    drivers::QEMUWriter qemu;
    qemu.set_global();

    log(Info, "KERNEL", "Entered kernel");
    while(1);
}