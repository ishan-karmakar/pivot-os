VIDEO_MEMORY equ 0xB8000
WHITE_ON_BLACK equ 0xF

print_string:
    pusha
    mov edx, VIDEO_MEMORY
.loop:
    mov al, [ebx]
    mov ah, WHITE_ON_BLACK
    cmp al, 0
    je .exit
    mov [edx], ax
    add ebx, 1
    add edx, 2
    jmp .loop
.exit:
    popa
    ret