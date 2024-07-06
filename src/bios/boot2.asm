; BIOS Bootloader stage 2
; Bootloader stage 1 will load this
%include "constants.asm"
%define VBE_INFO 0x8000
%define VBE_MINFO 0x8200
%define FAT_LOAD_ADDR 0x8000
%define FAT_START 34
%define SECTORS_PER_FAT 2

[bits 16]
[org BIOS2_ORG]
mov bx, entered_bl
call print
call find_video_mode
call parse_fat
jmp $

find_video_mode:
    mov dword [VBE_INFO + vbe_info.signature], 'VBE2'
    mov ax, 0x4F00
    mov di, VBE_INFO
    int 0x10
    mov bx, svga_err
    cmp ax, 0x4F
    jne error
    mov si, [VBE_INFO + vbe_info.video_modes]
    mov fs, [VBE_INFO + vbe_info.video_modes + 2]
.find_mode:
    mov dx, [fs:si]
    add si, 2
    mov bx, video_mode_err
    cmp dx, 0xFFFF
    je error

    mov ax, 0x4F01
    mov cx, dx
    mov di, VBE_MINFO
    int 0x10

    mov bx, svga_err
    cmp ax, 0x4F
    jne error

    mov ax, [VBE_MINFO + vbe_minfo.attributes]
    test ax, 1 << 7
    jz .find_mode

    mov ax, [VBE_MINFO + vbe_minfo.width]
    cmp ax, TARGET_WIDTH
    jne .find_mode

    mov ax, [VBE_MINFO + vbe_minfo.height]
    cmp ax, TARGET_HEIGHT
    jne .find_mode

    mov al, [VBE_MINFO + vbe_minfo.bpp]
    cmp al, TARGET_BPP
    jne .find_mode

    mov al, [VBE_MINFO + vbe_minfo.memory_model]
    cmp al, TARGET_MEMORY_MODEL
    jne .find_mode

    ; Found valid video mode
    mov ax, 0x4F02
    mov bx, dx
    or bx, 0x4000
    int 0x10

    mov bx, svga_err
    cmp ax, 0x4F
    jne error

    mov bx, set_video_mode
    call print
    ret

parse_fat:
    ; First load first sector
    mov word [dap + dap_t.num_sectors], 1
    mov dword [dap + dap_t.buffer], FAT_LOAD_ADDR
    mov dword [dap + dap_t.low_lba], FAT_START
    call load_sectors
    mov bx, disk_read_err
    cmp word [FAT_LOAD_ADDR + 510], 0xAA55
    jne error

    ; Calculate sector of root directory entry
    ; FAT_START + FAT entries * 2 + Reserved sectors
    mov edx, FAT_START
    add dx, [FAT_LOAD_ADDR + 0xE] ; Reserved sectors
    ; TODO: I guess something bad will happen if it overflows, but rn I don't care
    ; Root directory entry
    xor ax, ax
    mov al, [FAT_LOAD_ADDR + 0x10]
    mov cx, SECTORS_PER_FAT
    push dx
    mul cx
    pop dx
    add dl, al

    mov dword [dap + dap_t.low_lba], edx
    call load_sectors
    mov dx, [FAT_LOAD_ADDR]
    call printh
    jmp $

%include "util16.asm"
%include "common.asm"

entered_bl db `Entered bootloader stage 2\r\n\0`
set_video_mode db `Set VESA video mode\r\n\0`
svga_err db `Error getting VESA info\0`
video_mode_err db `Could not find suitable video mode\0`

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
endstruc