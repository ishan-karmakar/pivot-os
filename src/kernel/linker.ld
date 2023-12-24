ENTRY (_start)

SECTIONS
{
	. = 1M;
    _kern_phys_start = .;
    # _kern_virt_offset = 0;
    _kern_virt_offset = 0xFFFFFFFF80000000;
    . += _kern_virt_offset;
    _kern_virt_start = .;

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
		*(.bss*)
	}

	.data ALIGN(4K) : AT (ADDR (.data) - _kern_virt_offset)
	{
		*(.data*)
	}

	_kern_end = .;
    _kern_phys_end = . - _kern_virt_offset;
}