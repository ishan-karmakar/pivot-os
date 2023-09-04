%define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
%define PAGE_SIZE 0x200000

[global ap_trampoline]
[bits 16]
ap_trampoline:
    cli
    cld
    jmp 0:0x8030

align 16
gdt32: ; 0x8010
    dq 0
    .code: ; 0x8018
        dw 0xFFFF
        dw 0
        db 0
        db 0b10011010
        db 0b11001111
        db 0
    .data: ; 0x8020
        dw 0xFFFF
        dw 0
        db 0
        db 0b10010010
        db 0b11001111
        db 0
    .pointer: ; 0x8028
        dw .pointer - gdt32 - 1
        dd 0x8010

align 16
load_gdt: ; 0x8030
    xor ax, ax
    mov ds, ax
    lgdt [0x8028]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x8:0x8050

align 32
[bits 32]
kernel32: ; 0x8050
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov byte [16 * PAGE_SIZE], 1
    jmp $