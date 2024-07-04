; BIOS Bootloader stage 2
; Bootloader stage 1 will load this

[bits 16]
[org 0x8100]
cli
lgdt [gdt32.pointer]
mov eax, cr0
or eax, 1
mov cr0, eax
jmp 0x8:init32

[bits 32]
init32:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebx, pm_msg
    call print_string
    jmp $

gdt32:
    dq 0
.code:
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0
.data:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
.pointer:
    dw .pointer - gdt32 - 1
    dd gdt32

pm_msg db `Loaded into 32-bit mode\0`

%include "screen32.asm"
