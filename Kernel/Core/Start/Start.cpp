#include "Start.hpp"
#include <Arch/Interface.hpp>
#include <Arch/x86_64/PIT.hpp>
#include <Core/MP/MP.hpp>
#include <Debug/kpanic.hpp>
#include <Drivers/IDE/IDE.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>
#include <Scheduler/Scheduler.hpp>
#include "Arch/x86_64/Interrupt/IRQDispatcher.hpp"
#include "Core/Error/Error.hpp"
#include "Daemons/Kbd/Kbd.hpp"
#include "Daemons/SysDbg/SysDbg.hpp"
#include "Daemons/Testd/Testd.hpp"
#include "Debug/kassert.hpp"
#include "Debug/klogf.hpp"
#include "LibFormat/Formatters/Pointer.hpp"
#include "LibGeneric/String.hpp"
#include "Memory/KHeap.hpp"
#include "Process/Process.hpp"

/** Start the kernel and boot into userland
 *
 * 	This is the main entrypoint for the non-platform specific kernel.
 */
[[noreturn, maybe_unused]] void core::start::start() {
	//  Meant to handle early debugging stuff, things that need to be
	//  init'd as soon as possible
	if(const auto err = arch::platform_early_init(); err != core::Error::Ok) {
		//  FIXME: Better error handling, break into debugger?
		kerrorf_static("[core::start] Platform early init failed with error code: {}\n", static_cast<size_t>(err));
		kpanic();
	}
	klogf_static("[core::start] Platform early init done\n");

	auto* env = core::mp::create_environment();
	arch::mp::environment_set(env);
	klogf_static("[core::start] Kernel starting on node={} with environment={x}\n", env->node_id, Format::ptr(env));

	//  Initialize required kernel subsystems
	VMM::initialize_kernel_vm();
	PMM::instance().init_deferred_allocators();
	KHeap::instance().init();

	//  Handles further platform initialization tasks
	if(const auto err = arch::platform_init(); err != core::Error::Ok) {
		//  FIXME: Better error handling, break into debugger?
		kerrorf_static("[core::start] Platform init failed with error code: {}\n", static_cast<size_t>(err));
		kpanic();
	}
	klogf("[core::start] Platform init done\n");
	klogf("[core::start] Kernel init done, time passed: {}ms\n", PIT::milliseconds());

	//  Prepare the next stage of initialization
	//  Late init is called within a multitasking environment, so it's
	//  safe to use more advanced locking primitives
	auto thread =
	        Process::create_with_main_thread(gen::String { "late_init" }, Process::kerneld(), core::start::late_init);
	core::mp::bootstrap_this_node(nullptr, thread.get());
}

/**
 * Perform late initialization of device drivers.
 * Late init is called within a proper multitasking environment within
 * a separate late_init kernel process.
 */
[[noreturn, maybe_unused]] void core::start::late_init() {
	//  Initialize drivers
	(void)driver::ide::init();

	//  Spawn the kernel serial debugger
	auto serial_dbg =
	        Process::create_with_main_thread(gen::String { "sys_dbg" }, Process::kerneld(), SysDbg::sysdbg_thread);
	this_cpu()->scheduler->run_here(serial_dbg.get());

	//  Spawn a demo thread that reads from the keyboard
	//  FIXME: Remove this, especially that we have to register the IRQ
	//  handler on our own instead of the thread doing it's own work.
	auto kbd = Process::create_with_main_thread(gen::String { "debug_keyboard" }, Process::kerneld(), Kbd::kbd_thread);
	kbd->sched_ctx().priority = 0;
	this_cpu()->scheduler->run_here(kbd.get());
	IRQDispatcher::register_microtask(1, Kbd::kbd_microtask);

	//  Spawn a demo userland "init" thread
	//  This is just a quick test to see if jumping to ring 3
	//  works properly, doesn't actually do any meaningful work.
	auto userland_init = Process::init();
	kassert(userland_init->vmm().clone_address_space_from(Process::kerneld()->vmm().pml4()));
	auto init_thread = Thread::create_in_process(userland_init, Testd::userland_test_thread);
	init_thread->sched_ctx().priority = 10;
	this_cpu()->scheduler->run_here(init_thread.get());

	klogf("[start::late_init] Late init completed\n");
	this_cpu()->scheduler->sleep();
	//  We should never be awoken, but put a safe guard anyway
	while(true)
		;
}
