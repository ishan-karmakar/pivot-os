[bits 64]

[global proc_wrapper]
[extern syscall]
proc_wrapper:
    call rax
    mov rdi, 60
    call syscall
