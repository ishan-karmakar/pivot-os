ENTRY (init_kernel)

SECTIONS
{
    _kern_virt_offset = 0xFFFFFFFF80000000;
    . = _kern_virt_offset;

	.text ALIGN (4K) : AT (ADDR (.text) - _kern_virt_offset)
	{
		*(.text*)
	}

	.rodata ALIGN(4K) : AT (ADDR (.rodata) - _kern_virt_offset)
	{
		*(.rodata*)
	}

	.bss ALIGN(4K) : AT (ADDR (.bss) - _kern_virt_offset)
	{
        *(COMMON)
		*(.bss*)
	}

	.data ALIGN(4K) : AT (ADDR (.data) - _kern_virt_offset)
	{
		*(.data*)
	}

    /DISCARD/ : {
        *(.note*)
        *(.common*)
    }
}