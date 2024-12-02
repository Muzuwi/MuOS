#pragma once
#include <SystemTypes.hpp>

///  Symbols from the linker script
extern unsigned char __BOOT0_PHYS_BASE;
extern unsigned char __BOOT0_PHYS_END;
extern unsigned char __BOOT0_VMKERNEL_START;

///  uKernel image header
///  This is assumed to follow the boot0 flat binary in physical memory.
///  If not found, boot0 will panic.
struct KernelImageHeader {
	uint64 magic;
	void* vmbase;
	uint64 vmsize;
	void (*vmentry)(void*);

	static constexpr uint64 MAGIC_VALUE = 0xF00DD00F;
};

enum PageTableEntryFlags {
	PTE_VALID = 1 << 0,
	PTE_READ = 1 << 1,
	PTE_WRITE = 1 << 2,
	PTE_EXECUTE = 1 << 3,
	PTE_ACCESSED = 1 << 6,
	PTE_DIRTY = 1 << 7,
	//  The following aren't actually PTE flags, but for simplicity
	//  they are included in the same enumeration.
	VM_2M = 1 << 8,
	VM_1G = 1 << 9,
};

static inline uint64 VPN2(uint64 addr) {
	return (addr >> 30) & 0x1FF;
}

static inline uint64 VPN1(uint64 addr) {
	return (addr >> 21) & 0x1FF;
}

static inline uint64 VPN0(uint64 addr) {
	return (addr >> 12) & 0x1FF;
}

static inline uint64 PPN2(uint64 addr) {
	return (addr >> 30) & 0x3FFFFFF;
}

static inline uint64 PPN1(uint64 addr) {
	return (addr >> 21) & 0x1FF;
}

static inline uint64 PPN0(uint64 addr) {
	return (addr >> 12) & 0x1FF;
}

constexpr size_t PAGE_SIZE = 4096;
constexpr size_t PAGE_SIZE_2M = 0x200000;
constexpr size_t PAGE_SIZE_1G = 0x40000000;
constexpr uint64 NEXT_LEVEL_MASK = 0x003FFFFFFFFFFC00;
constexpr uint64 NEXT_LEVEL_SHIFT = 2;
constexpr auto DEFAULT_FLAGS = (PageTableEntryFlags)(PTE_READ | PTE_WRITE | PTE_EXECUTE);

#define IS_POW2_ALIGNED(powof2, value) ((reinterpret_cast<uintptr_t>(value) & (powof2 - 1)) == 0)

#define read_csr(reg)                                 \
	({                                                \
		uint64_t __tmp;                               \
		asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
		__tmp;                                        \
	})

#define write_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"rK"(val)); })

///  Map a physical address to the given virtual address.
///  This affects the bootstrap mappings that are prepared by boot0
///  before booting into the proper kernel. This function will panic
///  on any failure, as it is assumed that any failures at this
///  stage are fatal.
void addrmap(void* paddr, void* vaddr, PageTableEntryFlags flags);
