ENTRY (init_kernel)

SECTIONS
{
    _kern_virt_offset = 0xFFFFFFFF80000000 + SIZEOF_HEADERS;
    . = _kern_virt_offset;

	.text ALIGN (0x1000) : AT (ADDR (.text) - _kern_virt_offset)
	{
		*(.text*)
        *(.gnu.linkonce.t*)
	}

    .cdtors ALIGN(0x1000) : AT (ADDR (.cdtors) - _kern_virt_offset)
    {
        __start_ctors = .;
        *(.init_array)
        *(SORT(.ctors*))
        __stop_ctors = .;

        __start_dtors = .;
        *(.fini_array)
        *(SORT(.dtors*))
        __stop_ctors = .;
    }

	.rodata ALIGN(0x1000) : AT (ADDR (.rodata) - _kern_virt_offset)
	{
		*(.rodata*)
        *(.gnu.linkonce.r*)
	}

	.data ALIGN(0x1000) : AT (ADDR (.data) - _kern_virt_offset)
	{
		*(.data*)
        *(.gnu.linkonce.d*)
	}

	.bss ALIGN(0x1000) : AT (ADDR (.bss) - _kern_virt_offset)
	{
        *(COMMON)
		*(.bss*)
        *(.gnu.linkonce.b*)
	}

    /DISCARD/ :
    {
        *(.note*)
        *(.comment*)
        *(.eh_frame*)
    }
}
