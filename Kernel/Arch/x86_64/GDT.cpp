#include <Arch/x86_64/GDT.hpp>
#include <Debug/klogf.hpp>

static uint8_t TSS[104];

static uint64_t _create_tss_higher_descriptor() {
	return (reinterpret_cast<uint64_t>(TSS)) >> 32u;
}

static uint64_t _create_tss_lower_descriptor() {
	const auto address = reinterpret_cast<uint64_t>(TSS);
	const auto base_h = (address & 0xFF000000) << 32u;
	const auto base_l = (address & 0x00FFFFFF) << 16u;
	const auto limit  = sizeof(TSS) & 0xFFFF;
	return base_h | 0x0000890000000000 | base_l | limit;
}

/*
	Descriptor table storage
*/
static uint64_t descriptor_table[16] {
	0x0,                        //  Null
	0x00209A0000000000,         //  Code0
	0x0000920000000000,         //  Data0
	0x0,                        //  Null
	0x0000F20000000000,         //  Data3
	0x0020FA0000000000,         //  Code3
	_create_tss_lower_descriptor(),    //  TSS_lower
	_create_tss_higher_descriptor(),    //  TSS_higher,
	0,0,0,0,0,0,0,0,
};


static struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdt_descriptor;

void GDT::init() {
	//  Set up the TSS
	for(auto& v : TSS) v = 0;
	*reinterpret_cast<uint16_t*>(&TSS[TSS_IOPB]) = sizeof(TSS) & 0x0000FFFF;

	gdt_descriptor.base = (uint64_t)descriptor_table;
	gdt_descriptor.limit = sizeof(descriptor_table);
	lgdt(&gdt_descriptor);

	//  Load TSS
	asm volatile(
            "ltr %0\n"
			::"r"((uint16_t)TSS_sel)
			);

	klogf_static("[GDT] TSS setup complete\n");
}

void GDT::set_irq_stack(void* stack) {
	*reinterpret_cast<uint64_t*>(&TSS[TSS_RSP0]) = reinterpret_cast<uint64_t>(stack);
}

extern "C" void loadGDT(void*);
void GDT::lgdt(void* ptr) {
	loadGDT(ptr);
}
