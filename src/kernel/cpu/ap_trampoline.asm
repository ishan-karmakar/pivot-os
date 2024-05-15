%define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
%define PAGE_SIZE 0x1000
%define HIGHER_HALF_OFFSET 0xFFFF800000000000
%define TRAMPOLINE_ADDR(addr) (0x8000 + (addr) - ap_trampoline)
section .text

align 4096
[global ap_trampoline]
[bits 16]
ap_trampoline:
    jmp 0:TRAMPOLINE_ADDR(kernel16)

align 16
pml4: dd 0
stack_top: dd 0
kgdtr: dq 0
kidtr: dq 0
ready: db 0

gdt32: ; 0x8010
    dq 0
    .code:
        dw 0xFFFF
        dw 0
        db 0
        db 0b10011010
        db 0b11001111
        db 0
    .data:
        dw 0xFFFF
        dw 0
        db 0
        db 0b10010010
        db 0b11001111
        db 0
    .pointer: ; 0x8028
        dw .pointer - gdt32 - 1
        dd TRAMPOLINE_ADDR(gdt32)

gdt64:
    dq 0
    .code:
        dw 0xFFFF
        dw 0
        db 0
        db 0b10011011
        db 0b00101111
        db 0
    .data:
        dw 0xFFFF
        dw 0
        db 0
        db 0b10010011
        db 0xF
        db 0
    .pointer:
        dw .pointer - gdt64 - 1
        dq TRAMPOLINE_ADDR(gdt64)

kernel16:
    cld
    cli

    ; Load 32 bit GDT
    lgdt [TRAMPOLINE_ADDR(gdt32.pointer)]

    ; Enable 32 bit
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Set CS and jump to 32 bit code
    jmp 0x8:TRAMPOLINE_ADDR(kernel32)

[bits 32]
kernel32: ; 0x8050
    ; Set segment registers to data descriptor
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Load PML4 into CR3
    mov eax, [TRAMPOLINE_ADDR(pml4)]
    mov cr3, eax

    ; Enable PAE paging
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set LM bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31 ; Enabling paging
    mov cr0, eax

    ; Jump to 64-bit code - We are still in 32-bit compatibility mode until we load GDT64
    jmp 0x8:TRAMPOLINE_ADDR(kernel32c)

[bits 64]
kernel32c: ; 32-bit compatibility until we load GDT64
    ; Load stack
    mov esp, [TRAMPOLINE_ADDR(stack_top)]

    ; Load OUR 64 bit GDT
    lgdt [TRAMPOLINE_ADDR(gdt64.pointer)]

    push 0x8
    push TRAMPOLINE_ADDR(kernel64)
    retfq

[bits 64]
kernel64: ; NOW we are in real 64 bit
    ; Set segment registers again
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set stack to higher half
    mov rax, HIGHER_HALF_OFFSET
    add rsp, rax

    mov rax, [TRAMPOLINE_ADDR(kgdtr)]
    lgdt [rax]
    
    push 0x8
    push ap_start
    retfq

[extern ap_kernel]
ap_start:
    mov rax, [TRAMPOLINE_ADDR(kidtr)]
    lidt [rax]
    sti
    call ap_kernel
    jmp $