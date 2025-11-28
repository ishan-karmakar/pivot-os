.global syscall
syscall:
    int $0x80
    ret

# IDTR stored in RDI
.global lidt_internal
lidt_internal:
    lidt (%rdi)
    ret

# GDTR stored in RDI
.global lgdt_internal
lgdt_internal:
    lgdt (%rdi)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    push $0x8
    push lgdt_continue
    lretq

lgdt_continue:
    ret