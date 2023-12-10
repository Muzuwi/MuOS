#include <Arch/Interface.hpp>
#include <Core/MP/MP.hpp>
#include <Debug/klogf.hpp>
#include <Memory/KHeap.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>

/** Bootstrap the current node for running the scheduler.
 *  This prepares the current node to run the scheduler main loop and
 *  enters it for the first time (presumably by switching to the idle task).
 *  `idle_task` MUST be an init thread, the scheduler will use this thread
 *  when no work is available. If not passed, the idle task will be created
 *  automatically.
 *  When `init` is passed, this init thread is added to the scheduler's runqueue.
 *  This can be used to perform additional initialization work in an MP environment.
 */
[[noreturn]] void core::mp::bootstrap_this_node(Thread* idle_task, Thread* init) {
	//  If this is a new node, set the kernel environment
	if(!arch::mp::environment_get()) {
		auto* core_env = core::mp::create_environment();
		arch::mp::environment_set(core_env);
	}
	klogf("[core::mp] Bootstrapping node {}\n", this_cpu()->node_id);

	//  Allocate a scheduler for the node
	this_cpu()->scheduler = KHeap::make<Scheduler>();
	if(init) {
		this_cpu()->scheduler->run_here(init);
	}
	//  Enter the scheduler
	this_cpu()->scheduler->bootstrap(idle_task);

	//  Should never be reached
	while(true)
		;
}
