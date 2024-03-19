; Code to jump from UEFI Bootloader to Kernel Entry
; Right now all this code does is set the RSP register
; I'm sure this code will be expanded on later...
; TODO: Load CR3 in here so no need to map all 4GB of lower half to make sure that boot loader was mapped
bits 64

section .text
global _start
extern init_kernel
_start:
    mov rsp, stack.top
    call init_kernel

section .bss
align 4096
global stack
stack:
    resb 16384
    .top:
