%include "src/boot/multiboot_struc.inc"
%define PAGE_TABLE_ENTRY 0b10000011
extern kernel_start
global start
global p4_table
global multiboot_framebuffer_data
global multiboot_basic_meminfo
global multiboot_acpi_info
global multiboot_mmap_data

section .text
[bits 32]
start:
    mov edi, ebx

    mov eax, p3_table
    or eax, 0b11
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table

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
    ; or eax, 1 << 16
    mov cr0, eax

    lgdt [gdt64.pointer]
    jmp (0x8):kernel_jumper

bits 64
kernel_jumper:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    lea rax, [rdi + 8]

read_multiboot:
    mov ebx, dword [rax + multiboot_tag.type]
    cmp ebx, MULTIBOOT_TAG_TYPE_FRAMEBUFFER
    je .mb_framebuffer

    cmp ebx, MULTIBOOT_TAG_TYPE_MMAP
    je .mb_mmap

    cmp ebx, MULTIBOOT_TAG_TYPE_BASIC_MEMINFO
    je .mb_meminfo

    cmp ebx, MULTIBOOT_TAG_TYPE_ACPI_OLD
    je .mb_acpi

    cmp ebx, MULTIBOOT_TAG_TYPE_ACPI_NEW
    je .mb_acpi

    jmp .skip_item
    ; TODO: Check if moving data variable into register and then setting register works
    .mb_framebuffer:
        mov [multiboot_framebuffer_data], rax
        jmp .skip_item

    .mb_mmap:
        mov [multiboot_mmap_data], rax
        jmp .skip_item

    .mb_meminfo:
        mov [multiboot_basic_meminfo], rax
        jmp .skip_item

    .mb_acpi:
        mov [multiboot_acpi_info], rax

    .skip_item:
        mov ebx, dword [rax + multiboot_tag.size]
        add rax, rbx
        add rax, 7
        and rax, ~7
        cmp dword [rax + multiboot_tag.type], MULTIBOOT_TAG_TYPE_END
        jne read_multiboot
        cmp dword [rax + multiboot_tag.size], 8
        jne read_multiboot

    mov rsp, stack.top
    call kernel_start

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096

align 4096
multiboot_framebuffer_data: resb 8
multiboot_mmap_data: resb 8
multiboot_basic_meminfo: resb 8
multiboot_acpi_info: resb 8
stack:
    resb 16384 ; 16 KB
    .top:

section .rodata
gdt64:
    dq 0
    .code equ $ - gdt64
        dq (1 << 44) | (1 << 47) | (1 << 41) | (1 << 43) | (1 << 53)

    .data equ $ - gdt64
        dq (1 << 44) | (1 << 47) | (1 << 41)

    .pointer:
        dw .pointer - gdt64 - 1
        dq gdt64
