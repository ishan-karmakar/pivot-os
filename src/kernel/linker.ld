ENTRY (init_kernel)

SECTIONS
{
    _page_size = 4K;
    _kern_virt_offset = 0xFFFFFFFF80000000;
    . = _kern_virt_offset;

	.text ALIGN (_page_size) : AT (ADDR (.text) - _kern_virt_offset)
	{
		*(.text*)
	}

	.rodata ALIGN(_page_size) : AT (ADDR (.rodata) - _kern_virt_offset)
	{
		*(.rodata*)
	}

	.bss ALIGN(_page_size) : AT (ADDR (.bss) - _kern_virt_offset)
	{
        *(COMMON)
		*(.bss*)
	}

	.data ALIGN(_page_size) : AT (ADDR (.data) - _kern_virt_offset)
	{
		*(.data*)
	}

    /DISCARD/ : {
        *(.note*)
        *(.common*)
    }
}