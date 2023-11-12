%define KERNEL_VIRTUAL_ADDR 0xFFFFFFFF80000000
%define PAGE_SIZE 0x200000
%define HIGHER_HALF_OFFSET 0xFFFF800000000000
%define CPU_ADDRESSES_ADDR (HIGHER_HALF_OFFSET + PAGE_SIZE)

[global ap_trampoline]
[bits 16]
ap_trampoline:
    cli
    cld
    xor ax, ax
    mov ds, ax
    lgdt [0x8038]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x8:0x8040

align 16
gdt32: ; 0x8020
    dq 0
    .code: ; 0x8028
        dw 0xFFFF
        dw 0
        db 0
        db 0b10011010
        db 0b11001111
        db 0
    .data: ; 0x8030
        dw 0xFFFF
        dw 0
        db 0
        db 0b10010010
        db 0b11001111
        db 0
    .pointer: ; 0x8038
        dw .pointer - gdt32 - 1
        dd 0x8020

[bits 32]
align 8
kernel32: ; 0x8040
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; mov dword [16 * PAGE_SIZE], 1
    mov eax, [15 * PAGE_SIZE + 4]
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load GDT
    mov eax, [15 * PAGE_SIZE]
    lgdt [eax]
    ; JMP to 64 bit
    jmp 0x8:0x8088

align 8
[bits 64]
kernel64: ; 0x8088
    mov bx, 0x10
    mov ss, bx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    sub rax, 10
    add rax, KERNEL_VIRTUAL_ADDR
    lgdt [rax]
    mov eax, [15 * PAGE_SIZE + 8]
    add rax, PAGE_SIZE
    mov rsp, rax
    push 0x8
    push 0x80C0
    retfq

align 8
[extern aps_running]
[extern ap_start]
ap_kernel:
    mov eax, [15 * PAGE_SIZE + 12]
    add rax, KERNEL_VIRTUAL_ADDR
    lidt [rax]
    sti
    lock inc byte [aps_running]
    call ap_start
    jmp $
