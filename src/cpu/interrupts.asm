[bits  64]
[extern interrupts_handler]

%macro isr 1
[global isr%1]
isr%1:
    ; When this macro is called the status registers are already on the stack
    call interrupts_handler ; Now we call the interrupt handler
    iretq ; Now we can return from the interrupt
%endmacro

%macro isr_err_code 1
[global isr%1]
isr%1:
    call interrupts_handler
    iretq
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