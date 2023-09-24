[bits  64]
[extern exception_handler]
[extern irq_handler]

%macro isr 1
[global isr%1]
isr%1:
    ; When this macro is called the status registers are already on the stack
    push 0
    push %1
    save_context
    mov rdi, rsp
    call exception_handler ; Now we call the interrupt handler
    restore_context
    iretq ; Now we can return from the interrupt
%endmacro

%macro isr_err_code 1
[global isr%1]
isr%1:
    push %1
    save_context
    mov rdi, rsp
    call exception_handler
    restore_context
    iretq
%endmacro

%macro irq 1
[global irq%1]
irq%1:
    mov rdi, %1
    call irq_handler
    iretq
%endmacro

%macro save_context 0
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro restore_context 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16 ; Remove error code and interrupt number from stack
%endmacro

%macro apic_eoi 0
    mov rdi, 0xB0
    mov rsi, 0
    call write_apic_register
%endmacro

[extern write_apic_register]

[extern apic_triggered]
[global irq32]
irq32:
    mov qword [apic_triggered], 1
    apic_eoi
    iretq

[extern handle_keyboard]
[global irq33]
irq33:
    call handle_keyboard
    apic_eoi
    iretq

[extern pit_ticks]
[global irq34]
irq34:
    inc qword [pit_ticks]
    apic_eoi
    iretq

[global irq35]
irq35:
    apic_eoi
    pop qword [return_address]
    pop qword [code_segment]
    pop qword [rflags]
    pop qword [stack_pointer]
    pop qword [stack_segment]
    mov rax, [active_task]
    call save_task_state
    inc qword [active_task]
    mov rax, [num_tasks]
    cmp [active_task], rax
    jl .load
    mov qword [active_task], 0
.load:
    mov rax, [active_task]
    mov rax, [tasks + rax * 8]
    call load_task_state
    push qword [stack_segment]
    push qword [stack_pointer]
    push qword [rflags]
    push qword [code_segment]
    push qword [return_address]
    mov rax, [rax_val]
    iretq

isr 0
isr 1
isr 2
isr 3
isr 4
isr 5
isr 6
isr 7
isr_err_code 8
isr 9
isr_err_code 10
isr_err_code 11
isr_err_code 12
isr_err_code 13
isr_err_code 14
isr 15
isr 16
isr_err_code 17
isr 18
isr 19
isr 20
isr 21
isr 22
isr 23
isr 24
isr 25
isr 26
isr 27
isr 28
isr 29
isr_err_code 30
isr 31
irq 255

struc TS
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
[global create_task]
save_task_state:
    push r15
    mov r15, rax
    mov rax, [stack_segment]
    mov [r15 + TS.ss], rax
    mov rax, [code_segment]
    mov [r15 + TS.cs], rax
    mov rax, [stack_pointer]
    mov [r15 + TS.rsp], rax
    mov rax, [return_address]
    mov [r15 + TS.rip], rax
    mov rax, [rflags]
    mov [r15 + TS.rflags], rax
    mov rax, cr3
    mov [r15 + TS.cr3], rax
    mov [r15 + TS.ds], ds
    mov [r15 + TS.es], es
    mov [r15 + TS.fs], fs
    mov [r15 + TS.gs], gs
    mov rax, [rax_val]
    mov [r15 + TS.rax], rax
    mov [r15 + TS.rbx], rbx
    mov [r15 + TS.rcx], rcx
    mov [r15 + TS.rdx], rdx
    mov [r15 + TS.rbp], rbp
    mov [r15 + TS.rsi], rsi
    mov [r15 + TS.rdi], rdi
    mov [r15 + TS.r8], r8
    mov [r15 + TS.r9], r9
    mov [r15 + TS.r10], r10
    mov [r15 + TS.r11], r11
    mov [r15 + TS.r12], r12
    mov [r15 + TS.r13], r13
    mov [r15 + TS.r14], r14
    pop qword [r15 + TS.r15]
    mov r15, [r15 + TS.r15]
    ret

load_task_state:
    ; rax contains address of task state
    mov r15, rax
    mov rax, [r15 + TS.ss]
    mov [stack_segment], rax
    mov rax, [r15 + TS.cs]
    mov [code_segment], rax
    mov rax, [r15 + TS.rsp]
    mov [stack_pointer], rax
    mov rax, [r15 + TS.rip]
    mov [return_address], rax
    mov rax, [r15 + TS.rflags]
    mov [rflags], rax
    mov rax, [r15 + TS.cr3]
    ; mov cr3, rax
    mov ds, [r15 + TS.ds]
    mov es, [r15 + TS.es]
    mov fs, [r15 + TS.fs]
    mov gs, [r15 + TS.gs]
    mov rax, [r15 + TS.rax]
    mov [rax_val], rax
    mov rbx, [r15 + TS.rbx]
    mov rcx, [r15 + TS.rcx]
    mov rdx, [r15 + TS.rdx]
    mov rbp, [r15 + TS.rbp]
    mov rsi, [r15 + TS.rsi]
    mov rdi, [r15 + TS.rdi]
    mov r8, [r15 + TS.r8]
    mov r9, [r15 + TS.r9]
    mov r10, [r15 + TS.r10]
    mov r11, [r15 + TS.r11]
    mov r12, [r15 + TS.r12]
    mov r13, [r15 + TS.r13]
    mov r14, [r15 + TS.r14]
    mov r15, [r15 + TS.r15]
    ret

create_task:
    ; rdi contains address of function
    ; rsi contains stack address for task
    mov [rax_val], rax
    pushfq
    pop qword [rflags]
    mov [code_segment], cs
    mov [stack_segment], ss
    mov [stack_pointer], rsi
    mov [return_address], rdi
    push rdi
    mov rdi, TS_size
    call kmalloc ; Pointer is stored in rax register
    mov rdi, [num_tasks]
    mov [tasks + rdi * 8], rax
    mov rax, [tasks + rdi * 8]
    pop rdi
    call save_task_state
    mov rax, [num_tasks]
    inc qword [num_tasks]
    ret

[global return_address]
stack_segment dq 0
stack_pointer dq 0
rflags dq 0
code_segment dq 0
return_address dq 0
rax_val dq 0
num_tasks dq 0
active_task dq 0
ticks dq 0
section .bss
tasks:
    resb 8 * 10