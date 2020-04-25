ENTRY(_start)

/*  Start of the virtual kernel address space*/
_ukernel_virtual_start = 0xD0100000;

/*  Physical address of the kernel */
_ukernel_physical_start = 0x100000;

/*	KMalloc section size */
_ukernel_kmalloc_size = 0x400000;

/*  Offset from virtual to physical*/
_ukernel_virtual_offset = _ukernel_virtual_start - _ukernel_physical_start;

SECTIONS
{
	. = _ukernel_virtual_start;
	/*  Start of kernel */
	_ukernel_start = .;

    _ukernel_RO_begin = .;
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
	_ukernel_RO_end = .;

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


	/*	KMalloc section  */
	.kmalloc ALIGN(4K) : AT(ADDR(.kmalloc) - _ukernel_virtual_offset)
	{
	    _ukernel_kmalloc_start = .;
		. += _ukernel_kmalloc_size;
	}
	_ukernel_kmalloc_end = .;
	
    .kernel_pages ALIGN(4K) : AT(ADDR(.kernel_pages) - _ukernel_virtual_offset)
	{
	    _ukernel_pages_start = .;
	    . += ((0x100000000 - _ukernel_virtual_offset) / 0x100000) * 1024;
	}


    _ukernel_interrupt_stack = .;
    .irq_stack ALIGN(4K) : AT(ADDR(.irq_stack) - _ukernel_virtual_offset)
    {
        . += 4096;
    }

    _ukernel_end = .;
}
