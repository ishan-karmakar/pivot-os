ENTRY (init_kernel)

SECTIONS
{
    _kern_virt_offset = 0xFFFFFFFF80000000;
    . = _kern_virt_offset;

	.text ALIGN (0x1000) : AT (ADDR (.text) - _kern_virt_offset)
	{
		*(.text*)
        *(.gnu.linkonce.t*)
	}

	.rodata ALIGN(0x1000) : AT (ADDR (.rodata) - _kern_virt_offset)
	{
        start_ctors = .;
        *(SORT(.ctors*))
        end_ctors = .;

        start_dtors = .;
        *(SORT(.dtors*))
        end_dtors = .;

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

    /DISCARD/ : {
        *(.note*)
        *(.common*)
        *(.eh_frame)
    }
}