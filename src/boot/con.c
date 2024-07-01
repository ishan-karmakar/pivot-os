#include <con.h>
#include <io/stdio.h>
#include <util/logger.h>
#include <stdarg.h>
static efi_so_t *con_out;

void efi_char_printer(char c) {
    uint16_t s[2] = { c, 0 };
    con_out->output_string(con_out, s);
    if (c == '\n')
        con_out->output_string(con_out, L"\r");
}

efi_status_t init_con(efi_so_t *cout) {
    con_out = cout;
    char_printer = efi_char_printer;

    efi_status_t status = con_out->reset(con_out, true);
    if (status < 0) return status;

    log(Info, "CON", "Initialized console output");
    return 0;
}