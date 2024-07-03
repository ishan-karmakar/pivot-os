; BIOS Bootloader stage 1
; BIOS will directly load this
; FIXME: Try to find a way to do this dynamically
%define BIOS2_SECTORS 1

[bits 16]
[org 0x7C00]

mov di, entered_bl
call print_string

mov sp, 0x8000
mov bp, sp

call check_lba_ext
call load_bios2
jmp $

load_bios2:
    mov si, disk_addr_packet
    mov ah, 0x42
    mov dl, 0x80
    mov di, disk_error
    int 0x13
    jc handle_error
    mov dx, [0x8000]
    call print_hex
    jmp $
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
    push di
    mov di, error_msg
    call print_string
    pop di
    call print_string
    jmp $

%include "screen.asm"

disk_addr_packet:
    db 16
    db 0
    dw BIOS2_SECTORS
    dw 0x8000
    dw 0
    dd 35
    dd 0

error_msg db 'Error: ', 0
entered_bl db 'Entered bootloader', 0xA, 0xD, 0
ext_lba_error db '1', 0xA, 0xD, 0
disk_error db '2', 0xA, 0xD, 0
TIMES 510 - ($ - $$) db 0
dw 0xAA55