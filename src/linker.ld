OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY (kinit)

PHDRS
{
    headers PT_PHDR PHDRS;
    text PT_LOAD FILEHDR PHDRS;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    _kern_virt_offset = 0xFFFFFFFF80000000;
    . = _kern_virt_offset + SIZEOF_HEADERS;

	.text ALIGN(CONSTANT(MAXPAGESIZE)) : {
		*(.text*)
        *(.gnu.linkonce.t*)
	} :text

	.rodata ALIGN(CONSTANT(MAXPAGESIZE)) :	{
		*(.rodata*)
        *(.gnu.linkonce.r*)
	} :rodata

	.data ALIGN(CONSTANT(MAXPAGESIZE)) : {
		*(.data*)
        *(.gnu.linkonce.d*)
	} :data

    .limine : {
        KEEP(*(.requests.start))
        KEEP(*(.requests))
        KEEP(*(.requests.end))
    } :data

    .ctors ALIGN(4) : {
        __start_ctors = .;
        *(.init_array)
        *(SORT(.ctors*))
        __stop_ctors = .;
    } :data

    .dtors ALIGN(4) : {
        __start_dtors = .;
        *(.fini_array)
        *(SORT(.dtors*))
        __stop_ctors = .;
    } :data

	.bss : {
		*(.bss*)
        *(COMMON)
	} :data

    /DISCARD/ :
    {
        *(.note*)
        *(.comment*)
        *(.eh_frame*)
        *(.interp)
    }
}
