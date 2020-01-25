#include <Arch/i386/GDT.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>

/*
	Descriptor table storage
*/
uint64_t GDT::descriptor_table[16];

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
	//  Set descriptors
	descriptor_table[0] = nulldescr;
	descriptor_table[1] = codedescr;
	descriptor_table[2] = datadescr;
	descriptor_table[3] = coder3descr;
	descriptor_table[4] = datar3descr;

#ifdef LEAKY_LOG
	kdebugf(GDT_LOG "GDT mem location 0x%x\n", (uint32_t)descriptor_table);
#endif

	uint16_t descr_size = sizeof(descriptor_table)*sizeof(uint64_t);
	descriptor_table[0] = (descr_size) | ((uint64_t)descriptor_table << 16);

	lgdt((uint32_t)descriptor_table);
	kdebugf(GDT_LOG "GDT set up complete with flat mapping\n");
}


/*
	Returns a qword representing a descriptor specified by parameters
	base, limit, and access+flag bytes
*/
uint64_t GDT::create_descriptor(uint32_t base, uint32_t limit, uint16_t acc){
	uint64_t descriptor = 0;

	descriptor |= limit & 0xFFFF;
	//printf("%x\n", limit & 0xFFFF);
	//printf("%x\n", descriptor & 0XffffFFFF);
	descriptor |= (uint64_t)(limit & 0xF0000) << 32;

	descriptor |= ((base & 0xFFFF) << 16);
	descriptor |= ((base & 0xFF0000) << 16);
	descriptor |= ((uint64_t)(base & 0xFF000000) << 32);

	// descriptor |= (uint64_t)(access) << 40;
	// descriptor |= (uint64_t)(flags & 0xC) << 52;
	descriptor |= ((uint64_t)(acc) & 0x00F0FF) << 40;
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