ENTRY(_start)

SECTIONS
{
	. = 1M;

	/*  Start of kernel */
	_ukernel_start = .;

	.text ALIGN(4K) : AT(ADDR(.text) - 0)
	{
		*(.multiboot)
		*(.text)
	}

	/*  Read-Only Data */
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0)
	{
		*(.rodata)

	}

	/*  R/W Data  */
	.data ALIGN(4K) : AT(ADDR(.data) - 0)
	{
		*(.data)

	}
	
	/*  Uninitialized Read-write data + stack*/
	.bss ALIGN(4K) : AT(ADDR(.bss) - 0)
	{
		*(COMMON)
		*(.bss)

	}

	_ukernel_end = .;

}