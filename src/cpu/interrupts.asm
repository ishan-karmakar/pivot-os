[bits 64]
[extern int_handler]
[extern fpu_sav]
[extern fpu_rest]
[extern fpu_storage_size]

%macro isr_base_code 1
    cli
    push %1
    save_context
    mov rdi, rsp
    call fpu_sav
    call int_handler ; Now we call the interrupt handler
    mov rdi, rax
    call fpu_rest
    mov rsp, rax
    restore_context
    iretq ; Now we can return from the interrupt
%endmacro

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

    isr_base_code %1
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
    push 0 ; void* for xsave
%endmacro

%macro restore_context 0
    pop r15 ; void* for xsave
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

; irq periodic_irq, periodic_handler
; irq apic_oneshot_irq, apic_oneshot_handler
; irq pit_irq, pit_handler
; irq rtc_irq, rtc_handler
; irq syscall_irq, syscall_handler
; irq keyboard_irq, keyboard_handler
; irq ipi_irq, ipi_handler
; irq spurious_irq, spurious_handler
; irq acpi_irq, acpi_handler
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
