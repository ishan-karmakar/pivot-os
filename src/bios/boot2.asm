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
    mov si, [0x8000 + vbe_info.video_modes]
    mov fs, [0x8000 + vbe_info.video_modes + 2]
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

    cmp ax, 0x4F
    jne error

    ; push dx
    ; mov dx, [0x8200 + vbe_minfo.framebuffer]
    ; call printh
    ; mov al, 32
    ; call printc
    ; mov dx, [0x8200 + vbe_minfo.framebuffer + 2]
    ; call printh
    ; mov al, 32
    ; call printc
    ; pop dx

    mov ax, [0x8200 + vbe_minfo.width]
    cmp ax, 0
    
    ; mov ax, [0x8200 + vbe_minfo.width]
    ; cmp ax, TARGET_WIDTH
    ; jne .find_mode

    ; mov ax, [0x8200 + vbe_minfo.height]
    ; cmp ax, TARGET_HEIGHT
    ; jne .find_mode

    ; mov al, [0x8200 + vbe_minfo.bpp]
    ; cmp al, TARGET_BPP
    ; jne .find_mode

    ; Found valid video mode, time to set mode
    jmp .find_mode
error:
    jmp $

svga_error db `Error getting VESA info\r\n\0`
good_error db `Good error\r\n\0`

%include "util.asm"
%include "util16.asm"

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
    .attributes resw 1 ; 0
    .window_a resb 1 ; 2
    .window_b resb 1 ; 3
    .granularity resw 1 ; 4
    .window_size resw 1 ; 6
    .segment_a resw 1 ; 8
    .segment_b resw 1 ; 10
    .win_func_ptr resd 1 ; 12
    .pitch resw 1 ; 16
    .width resw 1 ; 18
    .height resw 1 ; 20
    .w_char resb 1 ; 22
    .h_char resb 1 ; 23
    .planes resb 1 ; 24
    .bpp resb 1 ; 25
    .banks resb 1 ; 26
    .memory_model resb 1 ; 27
    .bank_size resb 1 ; 28
    .image_pages resb 1 ; 29
    .rsv resb 1 ; 30
    .red_mask resb 1 ; 31
    .red_position resb 1 ; 32
    .green_mask resb 1 ; 33
    .green_position resb 1 ; 34
    .blue_mask resb 1 ; 35
    .blue_position resb 1 ; 36
    .rsv_mask resb 2 ; 38
    .direct_color_attributes resb 1 ; 39
    .framebuffer resd 1 ; 40
    .off_screen_mem_off resd 1
    .off_screen_mem_size resw 1
    resb 206
endstruc