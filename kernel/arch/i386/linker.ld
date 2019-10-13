ENTRY(_start)

SECTIONS
{
	/*  Start at 1MiB */
	. = 1M;

	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	/*  Read-Only Data */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)

	}

	/*  R/W Data  */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)

	}
	
	/*  Uninitialized Read-write data + stack*/
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)

	}

}