#include <Arch/VM.hpp>
#include <Arch/x86_64/Boot/MultibootInfo.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Interrupt/IDT.hpp>
#include <Arch/x86_64/Interrupt/PIC8259.hpp>
#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Arch/x86_64/PIT.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/Mem/Layout.hpp>
#include <Core/MP/MP.hpp>
#include <Core/Start/Start.hpp>
#include <SystemTypes.hpp>

//  Low conventional memory is marked as reserved for kernel purposes
//	This specifies the cutoff start address, where memory regions
//  that start at base addresses less than this value will be reserved.
#define CONFIG_ARCH_X86_64_LOWMEM_RESERVE_CUTOFF ((void*)0x10000)

CREATE_LOGGER("boot::grub", core::log::LogLevel::Debug);

static PhysPtr<MultibootInfo> s_multiboot_context;
static void platform_boot_grub_init_memory(MultibootInfo* bootinfo);

extern "C" [[noreturn, maybe_unused]] void platform_boot_entry(void* context) {
	s_multiboot_context = PhysPtr<MultibootInfo> { (MultibootInfo*)context };

	//  Initialize the execution environment
	//  This must be done as soon as possible
	void* env = arch::mp::create_environment();
	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);
	this_execution_environment()->gdt.load();
	x86_64::pic8259_init();
	IDT::init();
	x86_64::pit_init();
	//  Force reload GSBASE after changing GDT
	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);

	//  Initialize the memory ranges provided to us by GRUB
	platform_boot_grub_init_memory(s_multiboot_context.get_mapped());
	//  Jump to the generic kernel
	core::start::start();

	ENSURE_NOT_REACHED();
}

static void platform_boot_grub_init_memory(MultibootInfo* bootinfo) {
	log.info("Multiboot memory map:");

	auto mmap_addr = bootinfo->mmap();
	auto mmap_end = bootinfo->mmap_end();

	//  Total system memory
	uint64_t memory_amount = 0, reserved_amount = 0;

	auto type_string = [](MultibootMMap::RegionType type) -> char const* {
		switch(type) {
			case MultibootMMap::RegionType::USABLE: return "usable";
			case MultibootMMap::RegionType::HIBERN: return "to be preserved";
			case MultibootMMap::RegionType::ACPI: return "acpi";
			case MultibootMMap::RegionType::BAD: return "defective";
			default: return "reserved";
		}
	};

	auto pointer = mmap_addr.get_mapped();
	while(pointer < mmap_end.get_mapped()) {
		auto start = pointer->start();
		auto range = pointer->range();
		auto end = start + range;

		log.info("|- {x} - {x}: {}", start, end, type_string(pointer->type()));

		switch(pointer->type()) {
			case MultibootMMap::RegionType::USABLE: {
				memory_amount += range;

				//  Reserve the lower conventional memory, as:
				//	1) There's usually machine-specific stuff down there that we may need
				//     that cannot be overwritten by normal page allocations
				//  2) We need to have some pages accessible in 16-bit reserved
				//	   to allow for booting other APs in the system
				//  3) The bootloader code will probably also use some conventional
				//	   for initial paging structures, which if overwritten will crash
				//	   the kernel catastrophically (this should realistically be fixed
				//	   by making the bootloader pass its allocation information to the kernel)
				//  Thus, prevent the use of these pages entirely for generic allocations
				//  as using them gives us barely any benefit over the amount of pain they
				//  could otherwise cause.
				if((void*)start < CONFIG_ARCH_X86_64_LOWMEM_RESERVE_CUTOFF) {
					(void)core::mem::create((void*)start, range, core::mem::RegionType::PlatformReservation);
					break;
				}

				auto kernel_phys_start = (uint64_t)&_ukernel_preloader_physical;
				auto kernel_phys_end = (uint64_t)&_ukernel_elf_end - (uint64_t)&_ukernel_virtual_offset;

				ENSURE((kernel_phys_start & 0xFFF) == 0);
				ENSURE((kernel_phys_end & 0xFFF) == 0);

				//  Split regions overlapping with the kernel executable
				if(start < kernel_phys_end) {
					if(end > kernel_phys_end) {
						if(start < kernel_phys_start) {
							(void)core::mem::create((void*)start, kernel_phys_start - start,
							                        core::mem::RegionType::Usable);
						}
						(void)core::mem::create((void*)kernel_phys_end, end - kernel_phys_end,
						                        core::mem::RegionType::Usable);
					}
				} else {
					(void)core::mem::create((void*)start, range, core::mem::RegionType::Usable);
				}

				break;
			}
			case MultibootMMap::RegionType::ACPI:
			case MultibootMMap::RegionType::HIBERN: {
				(void)core::mem::create((void*)start, range, core::mem::RegionType::HardwareReservation);
				break;
			}
			case MultibootMMap::RegionType::BAD: {
				(void)core::mem::create((void*)start, range, core::mem::RegionType::Defective);
				//  explicit empty case
				break;
			}
			default: {
				reserved_amount += range;
				(void)core::mem::create((void*)start, range, core::mem::RegionType::HardwareReservation);
				break;
			}
		}

		pointer = pointer->next_entry();
	}
	const auto kernel_start = (uint64_t)(&_ukernel_elf_start), kernel_end = (uint64_t)(&_ukernel_elf_end);
	const uint32_t mem_mib = memory_amount / 0x100000;

	log.info("Kernel-used memory: {} KiB", (kernel_end - kernel_start) / 0x1000);
	log.info("Total usable memory: {} MiB", mem_mib);
	log.info("Reserved memory: {} bytes", reserved_amount);
}
