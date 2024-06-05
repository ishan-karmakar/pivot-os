bits 64
global load_gdt
load_gdt:
    ; Address of gdtr is in RDI
    lgdt [rdi]
    push 0x8
    push loaded_gdt
    retfq

loaded_gdt:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret