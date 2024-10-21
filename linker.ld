PHDRS
{
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text (ALIGN(CONSTANT(MAXPAGESIZE))) : {
        *(.text .text.*)
    } :text

    .rodata (ALIGN(CONSTANT(MAXPAGESIZE))) : {
        *(.rodata .rodata.*)
    } :rodata

    .init_array ALIGN(8) : {
        KEEP(*(SORT_BY_INIT_PRIORITY(.init.array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        PROVIDE_HIDDEN(__init_array_end = .);
    }

    .fini_array ALIGN(8) : {
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }

    .data (ALIGN(CONSTANT(MAXPAGESIZE))) : {
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data
}