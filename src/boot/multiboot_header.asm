section .multiboot_header
header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

framebuffer_tag_start:
    dw 0x5
    dw 0x0
    dd framebuffer_tag_end - framebuffer_tag_start
    dd 0
    dd 0
    dd 0
framebuffer_tag_end:
    align 8
	dw 0
	dw 0
	dd 8
header_end: