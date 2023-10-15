struc EF
    .rip resq 1
    .rax resq 1
    .rbx resq 1
    .rcx resq 1
    .rdx resq 1
    .rbp resq 1
    .rsi resq 1
    .rdi resq 1
    .rsp resq 1
    .r8 resq 1
    .r9 resq 1
    .r10 resq 1
    .r11 resq 1
    .r12 resq 1
    .r13 resq 1
    .r14 resq 1
    .r15 resq 1
    .cr3 resq 1
    .rflags resq 1 ; 152
    .cs resw 1
    .ds resw 1
    .ss resw 1
    .es resw 1
    .fs resw 1
    .gs resw 1 ; 164
endstruc

[extern kmalloc]
[extern root_thread]
[extern active_thread]
[global save_ef]
save_ef:
    push r15
    mov r15, rax
    mov rax, [stack_segment]
    mov [r15 + EF.ss], rax
    mov rax, [code_segment]
    mov [r15 + EF.cs], rax
    mov rax, [stack_pointer]
    mov [r15 + EF.rsp], rax
    mov rax, [return_address]
    mov [r15 + EF.rip], rax
    mov rax, [rflags]
    mov [r15 + EF.rflags], rax
    mov [r15 + EF.rbp], rbp
    mov rax, cr3
    mov [r15 + EF.cr3], rax
    mov [r15 + EF.ds], ds
    mov [r15 + EF.es], es
    mov [r15 + EF.fs], fs
    mov [r15 + EF.gs], gs
    mov rax, [rax_val]
    mov [r15 + EF.rax], rax
    mov [r15 + EF.rbx], rbx
    mov [r15 + EF.rcx], rcx
    mov [r15 + EF.rdx], rdx
    mov [r15 + EF.rsi], rsi
    mov [r15 + EF.rdi], rdi
    mov [r15 + EF.r8], r8
    mov [r15 + EF.r9], r9
    mov [r15 + EF.r10], r10
    mov [r15 + EF.r11], r11
    mov [r15 + EF.r12], r12
    mov [r15 + EF.r13], r13
    mov [r15 + EF.r14], r14
    pop qword [r15 + EF.r15]
    mov r15, [r15 + EF.r15]
    ret

[global load_ef]
load_ef:
    ; rax contains address of task state
    mov r15, rax
    mov rax, [r15 + EF.ss]
    mov [stack_segment], rax
    mov rax, [r15 + EF.cs]
    mov [code_segment], rax
    mov rax, [r15 + EF.rsp]
    mov [stack_pointer], rax
    mov rax, [r15 + EF.rip]
    mov [return_address], rax
    mov rax, [r15 + EF.rflags]
    mov [rflags], rax
    ; mov rax, [r15 + EF.cr3]
    ; mov cr3, rax
    mov ds, [r15 + EF.ds]
    mov es, [r15 + EF.es]
    mov fs, [r15 + EF.fs]
    mov gs, [r15 + EF.gs]
    mov rax, [r15 + EF.rax]
    mov [rax_val], rax
    mov rbx, [r15 + EF.rbx]
    mov rcx, [r15 + EF.rcx]
    mov rdx, [r15 + EF.rdx]
    mov rbp, [r15 + EF.rbp]
    mov rsi, [r15 + EF.rsi]
    mov rdi, [r15 + EF.rdi]
    mov r8, [r15 + EF.r8]
    mov r9, [r15 + EF.r9]
    mov r10, [r15 + EF.r10]
    mov r11, [r15 + EF.r11]
    mov r12, [r15 + EF.r12]
    mov r13, [r15 + EF.r13]
    mov r14, [r15 + EF.r14]
    mov r15, [r15 + EF.r15]
    ret

[extern thread_wrapper]
[global create_thread_ef]
create_thread_ef:
    ; rdi contains address of function
    ; rsi contains stack address for task
    mov [rax_val], rax
    pushfq
    pop qword [rflags]
    mov [code_segment], cs
    mov [stack_segment], ss
    mov [stack_pointer], rsi
    mov qword [return_address], thread_wrapper
    push rdi
    mov rdi, EF_size
    call kmalloc ; Pointer is stored in rax register
    pop rdi
    push rax
    call save_ef
    pop rax
    ret

[global stack_segment]
[global stack_pointer]
[global rflags]
[global code_segment]
[global return_address]
[global rax_val]
[global sleeping]
stack_segment dq 0
stack_pointer dq 0
rflags dq 0
code_segment dq 0
return_address dq 0
rax_val dq 0
sleeping db 0