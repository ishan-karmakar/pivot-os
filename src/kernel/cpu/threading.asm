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
    .rflags resq 1
    .cs resw 1
    .ds resw 1
    .ss resw 1
    .es resw 1
    .fs resw 1
    .gs resw 1
endstruc

[extern kmalloc]
[extern root_thread]
[extern active_thread]
[global save_ef]
save_ef:
    push r15
    mov r15w, [stack_segment]
    mov [rax + EF.ss], r15w
    mov r15w, [code_segment]
    mov [rax + EF.cs], r15w
    mov r15, [stack_pointer]
    mov [rax + EF.rsp], r15
    mov r15, [return_address]
    mov [rax + EF.rip], r15
    mov r15, [rflags_reg]
    mov [rax + EF.rflags], r15
    mov [rax + EF.rbp], rbp
    mov r15, cr3
    mov [rax + EF.cr3], r15
    mov [rax + EF.ds], ds
    mov [rax + EF.es], es
    mov [rax + EF.fs], fs
    mov [rax + EF.gs], gs
    mov r15, [rax_val]
    mov [rax + EF.rax], r15
    mov [rax + EF.rbx], rbx
    mov [rax + EF.rcx], rcx
    mov [rax + EF.rdx], rdx
    mov [rax + EF.rsi], rsi
    mov [rax + EF.rdi], rdi
    mov [rax + EF.r8], r8
    mov [rax + EF.r9], r9
    mov [rax + EF.r10], r10
    mov [rax + EF.r11], r11
    mov [rax + EF.r12], r12
    mov [rax + EF.r13], r13
    mov [rax + EF.r14], r14
    pop qword [r15 + EF.r15]
    mov r15, [r15 + EF.r15]
    ret

[global load_ef]
[extern testt]
load_ef:
    ; rax contains address of task state
    mov r15w, [rax + EF.ss]
    mov [stack_segment], r15w
    mov r15w, [rax + EF.cs]
    mov [code_segment], r15w
    mov r15, [rax + EF.rsp]
    mov [stack_pointer], r15
    mov r15, [rax + EF.rip]
    mov [return_address], r15
    push rax
    mov rdi, r15
    call testt
    pop rax
    mov r15, [rax + EF.rflags]
    mov [rflags_reg], r15
    ; mov rax, [rax + EF.cr3]
    ; mov cr3, rax
    mov ds, [rax + EF.ds]
    mov es, [rax + EF.es]
    mov fs, [rax + EF.fs]
    mov gs, [rax + EF.gs]
    mov r15, [rax + EF.rax]
    mov [rax_val], r15
    mov rbx, [rax + EF.rbx]
    mov rcx, [rax + EF.rcx]
    mov rdx, [rax + EF.rdx]
    mov rbp, [rax + EF.rbp]
    mov rsi, [rax + EF.rsi]
    mov rdi, [rax + EF.rdi]
    mov r8, [rax + EF.r8]
    mov r9, [rax + EF.r9]
    mov r10, [rax + EF.r10]
    mov r11, [rax + EF.r11]
    mov r12, [rax + EF.r12]
    mov r13, [rax + EF.r13]
    mov r14, [rax + EF.r14]
    mov r15, [rax + EF.r15]
    ret

[extern thread_wrapper]
[global create_thread_ef]
create_thread_ef:
    ; rdi contains address of function
    ; rsi contains stack address for task
    mov [rax_val], rax
    pushfq
    pop qword [rflags_reg]
    mov [code_segment], cs
    mov [stack_segment], ss
    mov [stack_pointer], rsi
    cmp rdx, 0
    je .no_thread_wrapper
    mov qword [return_address], thread_wrapper
    jmp .both
.no_thread_wrapper:
    mov [return_address], rdi
.both:
    push rdi
    mov rdi, EF_size
    call kmalloc ; Pointer is stored in rax register
    pop rdi
    call save_ef
    ret

[global failsafe_thread_fn]
failsafe_thread_fn:
    pause
    jmp failsafe_thread_fn

[global scheduler_yield]
scheduler_yield:
    int 0x20
    ret

[global stack_segment]
[global stack_pointer]
[global rflags_reg]
[global code_segment]
[global return_address]
[global rax_val]
stack_segment dq 0
stack_pointer dq 0
rflags_reg dq 0
code_segment dq 0
return_address dq 0
rax_val dq 0