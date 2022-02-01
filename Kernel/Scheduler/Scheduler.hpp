#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Kernel/Scheduler/RunQueue.hpp>
#include "Daemons/BootAP/BootAP.hpp"

class Thread;

class Scheduler {
	friend void BootAP::boot_ap_thread();

	RunQueue m_rq;
	Thread* m_ap_idle;

	static unsigned pri_to_quants(uint8_t priority);
	Thread* create_idle_task();
	void add_thread_to_rq(Thread*);
public:
	void bootstrap();
	void tick();
	void schedule();
	void interrupt_return_common();

	void wake_up(Thread*);
};