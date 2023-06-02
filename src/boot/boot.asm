extern kernel_start
global start
section .text
bits 32
start:
    mov esp, stack.top
    ; Check multiboot
    cmp eax, 0x36d76289
    jne error

    ; Check cpuid
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je error

    ; Check long mode
    mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb error

	mov eax, 0x80000001
	cpuid
	test edx, 1 << 29
	jz error

    ; Setup page tables
    mov eax, p3_table
    or eax, 0b11
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    mov ecx, 0
.loop:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .loop

    mov eax, p4_table
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

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

error:
    cli
    hlt

bits 64
long_mode_start:
    call kernel_start

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096

stack:
    resb 4096 * 4
    .top:

section .rodata
gdt64:
    dq 0
    .code: equ $ - gdt64
        dq (1 <<44) | (1 << 47) | (1 << 41) | (1 << 43) | (1 << 53)
    .data: equ $ - gdt64
        dq (1 << 44) | (1 << 47) | (1 << 41)
    .pointer:
        dw $ - gdt64 - 1
        dq gdt64
