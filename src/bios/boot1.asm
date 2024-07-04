; BIOS Bootloader stage 1
; BIOS will directly load this
%include "constants.asm"

[bits 16]
[org 0x7C00]
mov di, entered_bl
call print_string

mov sp, 0x8000
mov bp, sp

jmp $
; call check_lba_ext
; call load_bios2
jmp BIOS2_ORG

load_bios2:
    mov si, disk_addr_packet
    mov ah, 0x42
    mov dl, 0x80
    mov di, disk_error
    int 0x13
    jc handle_error
    ret

check_lba_ext:
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    mov di, ext_lba_error
    int 0x13
    jc handle_error
    ret

handle_error:
    call print_string
    jmp $

disk_addr_packet:
    db 16
    db 0
    dw BIOS2_SECTORS
    dw BIOS2_ORG
    dw 0
    dd BIOS2_START_SEC
    dd 0

%include "screen16.asm"

entered_bl db `Entered bootloader\r\n\0`
ext_lba_error db `BIOS does not support LBA Ext Read\r\n\0`
disk_error db `Disk read error\r\n\0`
TIMES 510 - ($ - $$) db 0
dw 0xAA55