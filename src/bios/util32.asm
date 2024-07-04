VIDEO_MEMORY equ 0xB8000
WHITE_ON_BLACK equ 0xF

printc:
    mov ah, WHITE_ON_BLACK
    mov edx, [video_memory]
    mov [edx], ax
    add edx, 2
    mov [video_memory], edx
    ret

video_memory dd 0xB8000