printc:
    push ax
    push bx
    mov ah, 0xE
    mov bh, 0
    mov bl, 1111111b
    int 0x10
    pop bx
    pop ax
    ret