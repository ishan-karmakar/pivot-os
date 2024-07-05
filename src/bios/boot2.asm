; BIOS Bootloader stage 2
; Bootloader stage 1 will load this
%include "constants.asm"

[bits 16]
[org BIOS2_ORG]
call find_video_mode

jmp $

find_video_mode:
    mov word [0x8000 + vbe_info.signature], 0x4256
    mov word [0x8000 + vbe_info.signature + 2], 0x3245
    mov ax, 0x4F00
    mov di, 0x8000
    int 0x10
    mov bx, svga_error
    cmp ax, 0x4F
    jne error
    mov fs, [0x8000 + vbe_info.video_modes]
    mov si, [0x8000 + vbe_info.video_modes + 2]
.find_mode:
    mov dx, [fs:si]
    add si, 2
    mov bx, svga_error
    cmp dx, 0xFFFF
    je error

    mov ax, 0x4F01
    mov cx, dx
    mov di, 0x8200
    int 0x10

    mov bx, svga_error
    cmp ax, 0x4F
    jne error

    mov ax, [0x8200 + vbe_minfo.width]
    cmp ax, TARGET_WIDTH
    jne .next_mode

    mov ax, [0x8200 + vbe_minfo.height]
    cmp ax, TARGET_HEIGHT
    jne .next_mode

    mov bx, good_error
    jmp error

.next_mode:
    add si, 2
    jmp .find_mode
    jmp $

svga_error db `Error getting VESA info\r\n\0`
good_error db `Good error\r\n\0`

struc vbe_info
    .signature resb 4
    .version resw 1
    .oem resd 1
    .capabilities resd 1
    .video_modes resd 1
    .video_mem resw 1
    .software_rev resw 1
    .vendor resd 1
    .product_name resd 1
    .product_rev resd 1
    resb 222
    resb 256
endstruc

struc vbe_minfo
    .attributes resw 1
    .window_a resb 1
    .window_b resb 1
    .granularity resw 1
    .window_size resw 1
    .segment_a resw 1
    .segment_b resw 1
    .win_func_ptr resd 1
    .pitch resw 1
    .width resw 1
    .height resw 1
    .w_char resb 1
    .y_char resb 1
    .planes resb 1
    .bpp resb 1
    .banks resb 1
    .memory_model resb 1
    .bank_size resb 1
    .image_pages resb 1
    resb 1
    .red_mask resb 1
    .red_position resb 1
    .green_mask resb 1
    .green_position resb 1
    .blue_mask resb 1
    .blue_position resb 1
    resb 2
    .direct_color_attributes resb 1
    .framebuffer resd 1
    .off_screen_mem_off resd 1
    .off_screen_mem_size resw 1
    resb 206
endstruc

%include "util.asm"
%include "util16.asm"