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

[extern save_ef]
[extern load_ef]
[extern stack_segment]
[extern stack_pointer]
[extern rflags]
[extern code_segment]
[extern return_address]
[extern rax_val]
[extern root_thread]
[extern active_thread]
[extern switch_next_ef]
[global irq35]
irq35:
    apic_eoi
    pop qword [return_address]
    pop qword [code_segment]
    pop qword [rflags]
    pop qword [stack_pointer]
    pop qword [stack_segment]
    mov rax, [active_thread]
    cmp rax, 0
    jne .thread_running
    mov rax, [root_thread]
    mov [active_thread], rax
    jmp .both
.thread_running:
    mov rax, [rax]
    call save_ef
    call switch_next_ef ; Now rax contains new active thread
.both:
    mov rax, [rax] ; Now rax, contains address of EF
    call load_ef
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