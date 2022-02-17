#pragma once
#include <Daemons/BootAP/BootAP.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Scheduler/RunQueue.hpp>
#include <SystemTypes.hpp>

class Thread;

class Scheduler {
	friend void BootAP::boot_ap_thread();
	friend class SMP;

	gen::Spinlock m_scheduler_lock;
	RunQueue m_rq;
	Thread* m_ap_idle;

	static unsigned pri_to_quants(uint8_t priority);
	Thread* create_idle_task(uint8 apic_id);
	void add_thread_to_rq(Thread*);
	void schedule_new();
public:
	void bootstrap();
	void tick();
	void schedule();
	void interrupt_return_common();

	void wake_up(Thread*);
	void block();
	void sleep();
	void dump_statistics();
};