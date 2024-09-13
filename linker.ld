OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kinit)

PHDRS
{
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text ALIGN(CONSTANT(MAXPAGESIZE)) : {
        *(.text .text.*)
    } :text

    .rodata ALIGN(CONSTANT(MAXPAGESIZE)) : {
        *(.rodata .rodata.*)
    } :rodata

    .data ALIGN(CONSTANT(MAXPAGESIZE)) : {
        *(.data .data.*)
        KEEP(*(.requests))
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}