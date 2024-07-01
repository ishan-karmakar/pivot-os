#include <stdint.h>
#include <boot/uefi.h>
#include <stdarg.h>

size_t
_IPrint (
    size_t                            Column,
    size_t                            Row,
    efi_simple_output_t              *Out,
    const uint16_t                     *fmt,
    const uint8_t                      *fmta,
    va_list                          args
    )
// Display string worker for: Print, PrintAt, IPrint, IPrintAt
{
    PRINT_STATE     ps;
    size_t            back;

    ZeroMem (&ps, sizeof(ps));
    ps.Context = Out;
    ps.Output  = (INTN (EFIAPI *)(VOID *, CHAR16 *)) Out->OutputString;
    ps.SetAttr = (INTN (EFIAPI *)(VOID *, size_t))  Out->SetAttribute;
    ps.Attr = Out->Mode->Attribute;

    back = (ps.Attr >> 4) & 0xF;
    ps.AttrNorm = EFI_TEXT_ATTR(EFI_LIGHTGRAY, back);
    ps.AttrHighlight = EFI_TEXT_ATTR(EFI_WHITE, back);
    ps.AttrError = EFI_TEXT_ATTR(EFI_YELLOW, back);

    if (fmt) {
        ps.fmt.pw = fmt;
    } else {
        ps.fmt.Ascii = TRUE;
        ps.fmt.pc = fmta;
    }

    va_copy(ps.args, args);

    if (Column != (size_t) -1) {
        uefi_call_wrapper(Out->SetCursorPosition, 3, Out, Column, Row);
    }

    back = _Print (&ps);
    va_end(ps.args);
    return back;
}


size_t Print(efi_simple_output_t *con_out, const uint16_t *fmt, ...) {
    va_list args;
    size_t back;
    va_start(args, fmt);
    back = IPrint(-1, -1, con_out, fmt, NULL, args);
    return back;
}

efi_status_t efi_main(void *image_handle, efi_system_table_t *st) {
    efi_status_t status;
    Print()
    status = st->con_out->reset(st->con_out, true);
    if (status < 0)
        return status;
    status = st->con_out->enable_cursor(st->con_out, true);
    if (status < 0) return status;
    st->con_out->output_string(st->con_out, L"Hello World\r\n");

    while (1);
}
