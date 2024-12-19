#include "Start.hpp"
#include <Arch/MP.hpp>
#include <Arch/Platform.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Mem/Layout.hpp>
#include <Core/MP/MP.hpp>
#include "Arch/x86_64/PIT.hpp"
#include "Core/Assert/Panic.hpp"
#include "Core/Error/Error.hpp"
#include "Core/IRQ/InterruptDisabler.hpp"
#include "Core/Log/Logger.hpp"
#include "Core/Mem/Heap.hpp"
#include "Core/Task/Scheduler.hpp"
#include "Core/Task/Task.hpp"
#include "LibFormat/Formatters/Pointer.hpp"
#include "LibGeneric/SharedPtr.hpp"

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

static void task_demo_main() {
	constexpr auto hashval = [](uint64_t value) -> uint64_t {
		struct xorshift64_state {
			uint64_t a;
		};
		constexpr auto xorshift64 = [](xorshift64_state& state) -> uint64_t {
			uint64_t x = state.a;
			x ^= x << 13;
			x ^= x >> 7;
			x ^= x << 17;
			return state.a = x;
		};
		auto state = xorshift64_state { .a = value };
		return xorshift64(state);
	};
	const auto current = this_cpu()->current_task();
	const auto sleep_duration = hashval(current->id) & 0xFFF;
	::log.info("Hello world from task {x}!", Format::ptr(current));

	while(true) {
		::log.info("Going to sleep for {}ms...", current->id, sleep_duration);
		PIT::sleep(sleep_duration);
		::log.info("Awoken", current->id);
	}
}

namespace core::start {
	[[noreturn]] void late_start();
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

	auto idle = core::task::make(platform_idle);
	if(!idle) {
		core::panic("Failed to create idle task");
	}
	auto late_start = core::task::make(core::start::late_start);
	if(!late_start) {
		core::panic("Failed to create late start task");
	}

	core::sched::enter_scheduler_for_the_first_time(idle.get(), late_start.get());
}

[[noreturn]] void core::start::late_start() {
	for(size_t i = 0; i < 100; ++i) {
		auto demo = core::task::make(task_demo_main);
		if(!demo) {
			core::panic("Failed to create demo task");
		}
		::log.debug("Created task={} tid={} ref={}", Format::ptr(demo.get()), demo->id, demo.use_count());
		core::sched::add_here(demo.get());
	}
	::log.info("Created tasks, late start done");
	core::sched::exit();
}
