ENTRY(_start)

/*  Start of the virtual kernel address space*/
_ukernel_virtual_start = 0xD0100000;

/*  Offset from virtual to physical*/
_ukernel_virtual_offset = _ukernel_virtual_start - 0x100000;

SECTIONS
{
	. = _ukernel_virtual_start;
	/*  Start of kernel */
	_ukernel_start = .;

	.text ALIGN(4K) : AT(ADDR(.text) - _ukernel_virtual_offset)
	{
		*(.multiboot)
		*(.text)
	}

	/*  Read-Only Data */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - _ukernel_virtual_offset)
	{
		*(.rodata)

	}

	/*  R/W Data  */
	.data ALIGN(4K) : AT(ADDR(.data) - _ukernel_virtual_offset)
	{
		*(.data)

	}
	
	/*  Uninitialized Read-write data + stack*/
	.bss ALIGN(4K) : AT(ADDR(.bss) - _ukernel_virtual_offset)
	{
		*(COMMON)
		*(.bss)

	}

	_ukernel_end = .;

}