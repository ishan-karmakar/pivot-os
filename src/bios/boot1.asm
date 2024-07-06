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
call load_bios2
jmp BIOS2_ORG

load_bios2:
    mov si, disk_addr_packet
    mov ah, 0x42
    mov dl, 0x80
    mov bx, disk_read_err
    int 0x13
    jc error
    ret

check_lba_ext:
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    mov bx, lba_ext_err
    int 0x13
    jc error
    ret

disk_addr_packet:
    db 16
    db 0
    dw BIOS2_SECTORS
    dw BIOS2_ORG
    dw 0
    dd BIOS2_START_SEC
    dd 0

%include "util.asm"
%include "util16.asm"

entered_bl db `Entered bootloader stage 1\r\n\0`
lba_ext_err db `LBA Ext not supported\0`
disk_read_err db `Disk read error\0`

TIMES 510 - ($ - $$) db 0
dw 0xAA55
