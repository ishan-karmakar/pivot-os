[bits 64]
[extern irq_handler]
[extern exception_handler]
[extern exception_handler_ec]

irq_common:
    ; push rax
    ; push rbx
    ; push rcx
    ; push rdx
    ; push rbp
    ; push rsi
    ; push rdi
    ; push r8
    ; push r9
    ; push r10
    ; push r11
    ; push r12
    ; push r13
    ; push r14
    ; push r15
    ; push 0
    mov rdi, rsp
    call irq_handler ; Now we call the interrupt handler
    mov rsp, rax
    ; pop rax
    ; cmp rax, 0
    ; je .no_switch
    ; mov cr3, rax
.no_switch:
    ; pop r15
    ; pop r14
    ; pop r13
    ; pop r12
    ; pop r11
    ; pop r10
    ; pop r9
    ; pop r8
    ; pop rdi
    ; pop rsi
    ; pop rbp
    ; pop rdx
    ; pop rcx
    ; pop rbx
    ; pop rax
    add rsp, 16 ; Remove error code and interrupt number from stack
    iretq ; Now we can return from the interrupt

exception_common:
    ; push rax
    ; push rbx
    ; push rcx
    ; push rdx
    ; push rbp
    ; push rsi
    ; push rdi
    ; push r8
    ; push r9
    ; push r10
    ; push r11
    ; push r12
    ; push r13
    ; push r14
    ; push r15
    mov rdi, rsp
    jmp exception_handler

exception_common_ec:
    ; push rax
    ; push rbx
    ; push rcx
    ; push rdx
    ; push rbp
    ; push rsi
    ; push rdi
    ; push r8
    ; push r9
    ; push r10
    ; push r11
    ; push r12
    ; push r13
    ; push r14
    ; push r15
    jmp exception_handler_ec

%macro exception 1
[global isr%1]
isr%1:
    push %1
    jmp exception_common
%endmacro

%macro exception_ec 1
[global isr%1]
isr%1:
    push %1
    jmp exception_common_ec
%endmacro

%macro irq 1
[global isr%1]
isr%1:
    push %1
    jmp irq_common
%endmacro

exception 0
exception 1
exception 2
exception 3
exception 4
exception 5
exception 6
exception 7
exception_ec 8
exception_ec 10
exception_ec 11
exception_ec 12
exception_ec 13
exception_ec 14
exception 16
exception_ec 17
exception 18
exception 19
exception 20
exception 28
exception_ec 29
exception_ec 30

%assign i 32
%rep 256 - 32
irq i
%assign i i + 1
%endrep

[section .rodata]
[global isr_table]
isr_table:
%assign i 0
%rep 256
%if i == 9 || i == 15 || i == 21 || (i >= 22 && i <= 27) || i == 31
dq 0
%else
dq isr%+i
%endif
%assign i i + 1
%endrep