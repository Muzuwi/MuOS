#include <Arch/i386/GDT.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>

extern uint32_t _ukernel_interrupt_stack;

/*
	Descriptor table storage
*/
uint64_t GDT::descriptor_table[16];

/*
 *  The task state segment
 */
uint32_t TSS[26];

/*
	Initializes a flat mapping of memory
*/
void GDT::init_GDT(){
	uint64_t nulldescr = create_descriptor(0, 0, 0),
			 codedescr = create_descriptor(0, 0xFFFFFFFF, 
			 			SEGMENT_DESCTYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SAVAIL(0) |
			 			SEGMENT_LONG(0) | SEGMENT_SIZE(1) | SEGMENT_GRAN(1) | 
			 			SEGMENT_PRIV(0) | SEG_CODE_XR ),
			 datadescr = create_descriptor(0, 0xFFFFFFFF, 
			 			SEGMENT_DESCTYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SAVAIL(0) |
			 			SEGMENT_LONG(0) | SEGMENT_SIZE(1) | SEGMENT_GRAN(1) | 
			 			SEGMENT_PRIV(0) | SEG_DATA_RW ),
			 coder3descr= create_descriptor(0, 0xFFFFFFFF,
			 			SEGMENT_DESCTYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SAVAIL(0) |
			 			SEGMENT_LONG(0) | SEGMENT_SIZE(1) | SEGMENT_GRAN(1) | 
			 			SEGMENT_PRIV(3) | SEG_CODE_XR ),
			 datar3descr = create_descriptor(0, 0xFFFFFFFF, 
			 			SEGMENT_DESCTYPE(1) | SEGMENT_PRESENT(1) | SEGMENT_SAVAIL(0) |
			 			SEGMENT_LONG(0) | SEGMENT_SIZE(1) | SEGMENT_GRAN(1) | 
			 			SEGMENT_PRIV(3) | SEG_DATA_RW );
	kdebugf("tss loc: %x\n", &TSS[0]);
	uint64_t tss = create_descriptor((uint32_t)&TSS[0], sizeof(TSS)-1, 0xc9);


	//  Set descriptors
	descriptor_table[0] = nulldescr;
	descriptor_table[kernelcdescr_offset] = codedescr;
	descriptor_table[kernelddescr_offset] = datadescr;
	descriptor_table[usercdescr_offset] = coder3descr;
	descriptor_table[userddescr_offset] = datar3descr;
	descriptor_table[tss_offset] = tss;

#ifdef LEAKY_LOG
	kdebugf(GDT_LOG "GDT mem location 0x%x\n", (uint32_t)descriptor_table);
#endif

	//  Ensure clear TSS
	for(auto& v : TSS)
		v = 0;

	//  Set up the TSS
	TSS[TSS_ESP0] = (uint32_t)&_ukernel_interrupt_stack;
	TSS[TSS_SS0] = kernel_DS;
	TSS[TSS_IOPB] = sizeof(TSS) & 0x0000FFFF;

	uint16_t descr_size = sizeof(descriptor_table);
	descriptor_table[0] = (descr_size) | ((uint64_t)descriptor_table << 16);

	lgdt((uint32_t)descriptor_table);
	kdebugf(GDT_LOG "GDT set up complete with flat mapping\n");

	//  Load TSS
	kdebugf("[gdt] TSS load\n");
	asm volatile(
            "ltr %0\n"
			::"r"((uint16_t)TSS_sel)
			);
}


/*
	Returns a qword representing a descriptor specified by parameters
	base, limit, and access+flag bytes
*/
uint64_t GDT::create_descriptor(uint32_t base, uint32_t limit, uint16_t acc){
	uint64_t descriptor = 0;

	descriptor |= limit & 0xFFFF;
	descriptor |= (uint64_t)(limit & 0xF0000) << 32;
	descriptor |= ((uint64_t)(base & 0xFFFF) << 16);
	descriptor |= ((uint64_t)(base & 0xFF0000) << 16);
	descriptor |= ((uint64_t)(base & 0xFF000000) << 32);
	descriptor |= ((uint64_t)(acc)) << 40;

#ifdef LEAKY_LOG
	kdebugf(GDT_LOG "Created descriptor base %x, limit %x, flags %x\n", base, limit, acc);
#endif
	return descriptor;
}

/*
	Given a 64-bit destination buffer and a GDT struct entry,
	stores a properly packed gdt entry at the destination
*/
void GDT::gdtentry_from_struct(uint16_t* dst, gdt_entry_t entry){
	dst[0] = 0;
	dst[1] = 0;
	dst[2] = 0;
	dst[3] = 0;

	dst[0] = (entry.limit & 0xFFFF);
	dst[1] = (entry.base & 0xFFFF);
	dst[2] = ((entry.base & 0xFF0000) >> 16) | ((entry.access) << 8);
	dst[3] = ((entry.base & 0xFF000000) >> 16) | ((entry.flags & 0xF) << 4) | ((entry.limit & 0xF0000) >> 16);
}