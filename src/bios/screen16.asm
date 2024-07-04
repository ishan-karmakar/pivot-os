; AL contains char to print
print_char:
    mov ah, 0xE
    mov bl, 7
    mov bh, 0
    int 0x10
    ret

print_string:
    mov al, [di]
    inc di
    or al, al
    jz .exit
    call print_char
    jmp print_string
.exit:
    ret