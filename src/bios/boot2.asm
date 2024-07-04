; BIOS Bootloader stage 2
; Bootloader stage 1 will load this
%include "constants.asm"

[bits 16]
[org BIOS2_ORG]
jmp $
; cli
; lgdt [gdt32.pointer]
; mov eax, cr0
; or eax, 1
; mov cr0, eax
; jmp 0x8:init32

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

vesa_info:
    db 'VESA' ; Signature
    dw 0 ; Version number
    dd 0 ; Pointer to OEM name
    dd 0 ; Capabilities
    dd 0 ; Supported VESA and OEM video modes
    dw 0 ; Amount of video memory in 64K blocks
    dw 0 ; OEM software version (BCD)
    dd 0 ; Pointer to vendor name
    dd 0 ; Pointer to product name
    dd 0 ; Pointer to product revision string
    dw 0 ; VBE/AF version (BCD)
    dd 0 ; Pointer to list of supported accelerated video modes
    times 216 db 0
    times 256 db 0

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
