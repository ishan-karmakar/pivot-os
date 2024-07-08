#include <uefi.h>
#include <con.h>
#include <io/stdio.h>
#include <util/logger.h>
#include <stdarg.h>

void efi_char_printer(char c) {
    uint16_t s[2] = { c, 0 };
    gST->con_out->output_string(gST->con_out, s);
    if (c == '\n')
        gST->con_out->output_string(gST->con_out, L"\r");
}

efi_status_t init_con(void) {
    char_printer = efi_char_printer;

    efi_status_t status = gST->con_out->reset(gST->con_out, true);
    if (EFI_ERR(status)) return status;

    log(Info, "CON", "Initialized console output");
    return 0;
}