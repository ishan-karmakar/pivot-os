; BIOS Bootloader stage 1
; BIOS will directly load this
%include "constants.asm"

[bits 16]
[org 0x7C00]
mov sp, 0x8000
mov bp, sp

call check_lba_ext
call load_bios2
jmp BIOS2_ORG

load_bios2:
    mov si, disk_addr_packet
    mov ah, 0x42
    mov dl, 0x80
    int 0x13
    jc error
    ret

check_lba_ext:
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    int 0x13
    jc error
    ret

disk_addr_packet:
    db 16
    db 0
    dw BIOS2_SECTORS
    dw BIOS2_ORG
    dw 0
    dd BIOS2_START_SEC
    dd 0

error:
    jmp $

%include "util.asm"
%include "util16.asm"

TIMES 510 - ($ - $$) db 0
dw 0xAA55


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
    .rsv0 resb 222
    .rsv1 resb 256
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
    .rsv0 resb 1
    .red_mask resb 1
    .red_position resb 1
    .green_mask resb 1
    .green_position resb 1
    .blue_mask resb 1
    .blue_position resb 1
    .rsv1 resb 2
    .direct_color_attributes resb 1
    .framebuffer resd 1
    .off_screen_mem_off resd 1
    .off_screen_mem_size resw 1
    .rsv2 resb 206
endstruc
