bits 64
global load_gdt
load_gdt:
    ; Address of gdtr is in RDI
    lgdt [rdi]
    push 0x8
    push load_tss
    retfq

load_tss:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rax, 0x28
    ltr ax
    ret