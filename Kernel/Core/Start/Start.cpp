#include "Start.hpp"
#include <Arch/MP.hpp>
#include <Arch/Platform.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Mem/Layout.hpp>
#include <Core/MP/MP.hpp>
#include "Core/Assert/Panic.hpp"
#include "Core/Error/Error.hpp"
#include "Core/Log/Logger.hpp"
#include "LibFormat/Formatters/Pointer.hpp"

CREATE_LOGGER("core::start", core::log::LogLevel::Debug);

/*	Dumps the physical memory layout and it's current kernel reservations.
 */
static void dump_core_mem_layout() {
	::log.info("Physical memory layout:");
	core::mem::for_each_region([](core::mem::Region region) {
		::log.info("|- {x} - {x} ({})", Format::ptr(region.start), Format::ptr((uint8*)region.start + region.len),
		           core::mem::type_to_str(region.type));
	});
}

/** Start the kernel and boot into userland
 *
 * 	This is the main entrypoint for the non-platform specific kernel.
 */
[[noreturn, maybe_unused]] void core::start::start() {
	//  Meant to handle early debugging stuff, things that need to be
	//  init'd as soon as possible
	if(const auto err = arch::platform_early_init(); err != core::Error::Ok) {
		::log.fatal("Platform early init failed with error code: {}", err);
		ENSURE_NOT_REACHED();
	}
	::log.info("Platform early init done");

	auto* env = core::mp::create_environment();
	arch::mp::environment_set(env);
	::log.info("Kernel starting on node={} with environment={x}", env->node_id, Format::ptr(env));

	dump_core_mem_layout();

	//  Handles further platform initialization tasks
	if(const auto err = arch::platform_init(); err != core::Error::Ok) {
		::log.fatal("Platform init failed with error code: {}", err);
		ENSURE_NOT_REACHED();
	}
	::log.info("Platform init done");

	core::panic("UNIMPLEMENTED: core::start finished");
}
