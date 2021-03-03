#pragma once
#include <Kernel/SystemTypes.hpp>

class Process;

class Scheduler {
	static unsigned pri_to_quants(uint8_t priority);
	static void rq_process_expire(Process*);
	static Process* rq_find_first_runnable();

public:
	static void init();
	static void tick();
	static void schedule();

	static void interrupt_return_common();
	static void notify_process_start(Process*);
	static void wake_up(Process*);
};