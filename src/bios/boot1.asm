; BIOS Bootloader stage 1
; BIOS will directly load this
%include "constants.asm"

[bits 16]
[org 0x7C00]
mov bx, entered_bl
call print
mov sp, 0x8000
mov bp, sp

call check_lba_ext
call get_bios2_sectors
call load_bios2
jmp BIOS2_ORG

; AX: Number of sectors B2 takes up
load_bios2:
    mov word [dap + dap_t.num_sectors], ax
    mov dword [dap + dap_t.buffer], BIOS2_ORG
    mov dword [dap + dap_t.low_lba], FAT_START + 1
    call load_sectors
    ret

; AX returns number of sectors B2 takes up
get_bios2_sectors:
    mov word [dap + dap_t.num_sectors], 1
    mov dword [dap + dap_t.buffer], BIOS2_ORG
    mov dword [dap + dap_t.low_lba], FAT_START
    call load_sectors
    mov ax, [BIOS2_ORG + 0xE]
    sub ax, 1
    ret

check_lba_ext:
    mov ah, 0x41
    mov bx, 0x55AA
    mov bx, lba_ext_err
    int 0x13
    jc error
    ret

%include "util16.asm"
%include "common.asm"

entered_bl db `Entered bootloader stage 1\r\n\0`
lba_ext_err db `LBA Ext not supported\0`

TIMES 510 - ($ - $$) db 0
dw 0xAA55