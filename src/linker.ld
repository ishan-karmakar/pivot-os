OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY (kinit)

PHDRS
{
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS {
    . = 0xFFFFFFFF80000000;

    .text (ALIGN(CONSTANT(MAXPAGESIZE))) : {
        *(.text .text.*)
    } :text

    .rodata (ALIGN(CONSTANT(MAXPAGESIZE))) : {
        *(.rodata .rodata.*)
    } :rodata

    .init_array ALIGN(8) : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);
    }

    .fini_array ALIGN(8) : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)));
        KEEP(*(.fini_array .dtors))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }

    .data : {
        *(.data .data.*)
        *(.requests.start)
        *(.requests)
        *(.requests.end)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data
}
