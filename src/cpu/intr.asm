[bits 64]
[extern int_handler]

%macro isr 1
[global isr%1]
isr%1:
%if !(%1 ==  8 || %1 == 10 || \
    %1 == 11 || %1 == 12 || \
    %1 == 13 || %1 == 14 || \
    %1 == 17 || %1 == 21 || \
    %1 == 29 || %1 == 30)
    push 0
%endif

    cli
    push %1
    save_context
    mov rdi, rsp
    call int_handler ; Now we call the interrupt handler
    mov rsp, rax
    restore_context
    iretq ; Now we can return from the interrupt
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

%assign i 0
%rep 256
isr i
%assign i i + 1
%endrep

[section .rodata]
[global isr_table]
isr_table:
%assign i 0
%rep 256
dq isr%+i
%assign i i + 1
%endrep
