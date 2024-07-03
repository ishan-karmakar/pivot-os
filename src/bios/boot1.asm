; BIOS Bootloader stage 1
; BIOS will directly load this

[bits 16]
[org 0x7C00]

mov si, entered_bl
call print_string

mov sp, 0x8000
mov bp, sp

mov bx, 0x8000
mov ah, 2
mov al, 1
mov cl, 1
mov ch, 0
mov dh, 0
int 0x13
jc derror
jmp $

derror:
    mov si, disk_error
    call print_string
    jmp $

%include "screen.asm"

entered_bl db 'Entered Bootloader', 0xA, 0xD, 0
disk_error db 'Disk read error', 0
TIMES 510 - ($ - $$) db 0
dw 0xAA55