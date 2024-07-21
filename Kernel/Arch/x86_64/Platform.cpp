#include <Arch/Interface.hpp>
#include <Arch/x86_64/Boot/MultibootInfo.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>
#include <Arch/x86_64/Interrupt.hpp>
#include <Arch/x86_64/MP/Boot.hpp>
#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Arch/x86_64/VGAConsole.hpp>
#include <Core/Error/Error.hpp>
#include <Core/MP/MP.hpp>
#include <Core/Start/Start.hpp>
#include <Memory/VMM.hpp>
#include <Syscalls/Syscall.hpp>
#include <SystemTypes.hpp>
#include "ACPI.hpp"
#include "APIC.hpp"
#include "Arch/x86_64/SerialConsole.hpp"
#include "Core/Log/Logger.hpp"
#include "Core/Mem/Layout.hpp"

CREATE_LOGGER("boot::grub", core::log::LogLevel::Debug);

static PhysPtr<MultibootInfo> s_multiboot_context;

static void platform_boot_grub_init_memory() {
	log.info("Multiboot memory map:");

	auto mmap_addr = s_multiboot_context->mmap();
	auto mmap_end = s_multiboot_context->mmap_end();

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

				//  Ignore everything in lowmem - we risk overwriting
				//  paging structures created by earlier bootloaders.
				if(end < 0x100000) {
					(void)core::mem::create((void*)start, range, core::mem::RegionType::HardwareReservation);
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

extern "C" [[noreturn, maybe_unused]] void platform_boot_entry(void* context) {
	//  Save the multiboot context
	gen::construct_at(&s_multiboot_context, PhysPtr<MultibootInfo>(reinterpret_cast<MultibootInfo*>(context)));
	//  Jump to the actual kernel boot
	core::start::start();
	//  Should never be reached
	while(true)
		;
}

core::Error arch::platform_early_init() {
	//  Initialize the execution environment
	//  This must be done as soon as possible
	void* env = arch::mp::create_environment();
	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);
	this_execution_environment()->gdt.load();
	IDT::init();
	//  Force reload GSBASE after changing GDT
	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);

	//  Initialize the memory ranges provided to us by GRUB
	platform_boot_grub_init_memory();

	(void)vgacon::init();
	Serial::init();
	(void)serialcon::init();
	CPU::initialize_features();

	return core::Error::Ok;
}

core::Error arch::platform_init() {
	irq_local_enable();

	PCI::discover();
	Syscall::init();
	ACPI::parse_tables();
	APIC::discover();
	arch::mp::boot_aps();

	return core::Error::Ok;
}

void arch::mp::environment_set(void* env) {
	this_execution_environment()->environment = static_cast<core::mp::Environment*>(env);
}

void* arch::mp::environment_get() {
	return this_execution_environment()->environment;
}
