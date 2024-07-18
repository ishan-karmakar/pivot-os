[bits 64]
[extern exception_handler]
[extern set_rsp0]

%macro isr_base_code 1
    push %1
    save_context
    mov rdi, rsp
    call exception_handler ; Now we call the interrupt handler
    restore_context
    iretq ; Now we can return from the interrupt
%endmacro

%macro isr 1
[global isr%1]
isr%1:
    ; When this macro is called the status registers are already on the stack
    push 0
    isr_base_code %1
%endmacro

%macro isr_err_code 1
[global isr%1]
isr%1:
    isr_base_code %1
%endmacro

%macro irq 2
[extern %2]
[global %1]
%1:
    push 0
    push 0
    save_context
    mov rdi, rsp
    call %2
    mov rsp, rax
    restore_context
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
irq apic_periodic_irq, apic_periodic_handler
; irq apic_oneshot_irq, apic_oneshot_handler
irq pit_irq, pit_handler
irq rtc_irq, rtc_handler
; irq syscall_irq, syscall_handler
; irq keyboard_irq, keyboard_handler
; irq ipi_irq, ipi_handler
; irq spurious_irq, spurious_handler