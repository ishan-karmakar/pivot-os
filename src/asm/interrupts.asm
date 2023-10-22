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
    push rdi
    push rsi
    mov rdi, 0xB0
    mov rsi, 0
    call write_apic_register
    pop rsi
    pop rdi
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
[extern active_thread]
[extern root_thread]
[extern next_thread]
[extern failsafe_thread]
[extern apic_ticks]
[extern sleeping]
[global irq35]
irq35:
    mov [rax_val], rax
    add qword [apic_ticks], 1
    apic_eoi
    pop qword [return_address]
    pop qword [code_segment]
    pop qword [rflags]
    pop qword [stack_pointer]
    pop qword [stack_segment]
    cmp qword [active_thread], 0
    jne .thread_running
    mov rax, [root_thread]
    mov [active_thread], rax
    jmp .load_ef
    
.thread_running:
    call next_thread
    cmp rax, [active_thread]
    je .active_thread
    cmp rax, [failsafe_thread]
    je .failsafe_thread
    jmp .not_active_thread

.same_thread:
    push qword [stack_segment]
    push qword [stack_pointer]
    push qword [rflags]
    push qword [code_segment]
    push qword [return_address]
    mov rax, [rax_val]
    iretq

.active_thread:
    cmp byte [sleeping], 0
    je .same_thread

.active_thread_sleeping:
    mov rax, [failsafe_thread]
    mov rax, [rax]
    call save_ef
    mov byte [sleeping], 0
    mov rax, [active_thread]
    jmp .load_ef ; Keeps loading failsafe rip even when switching back

.failsafe_thread:
    cmp byte [sleeping], 1
    je .same_thread

.failsafe_thread_switch:
    mov byte [sleeping], 1
    mov rax, [active_thread]
    mov rax, [rax]
    call save_ef
    mov rax, [failsafe_thread]
    jmp .load_ef

.not_active_thread:
    push rax
    mov rax, [active_thread]
    mov rax, [rax]
    call save_ef
    pop rax
    mov [active_thread], rax
    jmp .load_ef

.load_ef:
    ; Thread pointer is in rax
    mov rax, [rax]
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
