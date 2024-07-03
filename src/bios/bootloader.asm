[bits 16]
[org 0x7C00]

mov si, hello_string
call print_string
jmp $

; AL contains char to print
print_char:
    mov ah, 0xE
    mov bl, 7
    mov bh, 0
    int 0x10
    ret

print_string:
    mov al, [si]
    call print_char
    inc si
    or al, al
    jnz print_string

    ret

hello_string db 'Hello World', 0
TIMES 510 - ($ - $$) db 0
dw 0xAA55