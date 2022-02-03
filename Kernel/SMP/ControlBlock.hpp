#pragma once
#include <SystemTypes.hpp>
#include <Scheduler/Scheduler.hpp>

class Thread;

class ControlBlock {
									//  Offset:
	ControlBlock* m_self_reference; //  0, store self reference for quick ctrlblock fetching
	Thread* m_thread;               //  8
	uint64   m_ap_id;               //  16
	uint64   _scratch;              //  24, only used in SysEntry to temporarily preserve user rsp
	Scheduler m_sched;
public:
	ControlBlock(uint8 ap_id) noexcept
	: m_self_reference(this), m_thread(nullptr), m_ap_id(ap_id), _scratch(0) {
		(void)m_self_reference;
		(void)_scratch;
	}

	Thread* current_thread() const {
		return m_thread;
	}

	void set_thread(Thread* thread) {
		m_thread = thread;
	}

	uint8 current_ap() const {
		return m_ap_id;
	}

	void set_ap(uint8 const id) {
		m_ap_id = id;
	}

	Scheduler& scheduler() {
		return m_sched;
	}
} __attribute__((packed));
