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
call svga_info
call load_bios2
jmp BIOS2_ORG

svga_info:
    mov dword [0x8000], 0x41534556
    mov ax, 0x4F00
    mov di, 0x8000
    int 0x10
    mov di, svga_error
    cmp ah, 1
    je error
    ret

load_bios2:
    mov si, disk_addr_packet
    mov ah, 0x42
    mov dl, 0x80
    mov bx, disk_error
    int 0x13
    jc error
    ret

check_lba_ext:
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    mov bx, ext_lba_error
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

entered_bl db `Entered bootloader\r\n\0`
ext_lba_error db `BIOS does not support LBA Ext Read\r\n\0`
disk_error db `Disk read error\r\n\0`
svga_error db `Error getting SVGA info\r\n\0`
TIMES 510 - ($ - $$) db 0
dw 0xAA55