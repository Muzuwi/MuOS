#pragma once

#include <Scheduler/Scheduler.hpp>
#include <SystemTypes.hpp>

class Thread;

struct GDT;

class ControlBlock {
	//  Offset:
	ControlBlock* m_self_reference;//  0, store self reference for quick ctrlblock fetching
	Thread* m_thread;              //  8
	uint64 m_ap_id;                //  16
	uint64 _scratch;               //  24, only used in SysEntry to temporarily preserve user rsp
	//  ----------------------------//
	Scheduler m_sched;
	GDT* m_gdt;
public:
	ControlBlock(uint8 ap_id) noexcept
	    : m_self_reference(this)
	    , m_thread(nullptr)
	    , m_ap_id(ap_id)
	    , _scratch(0)
	    , m_gdt(nullptr) {
		(void)m_self_reference;
		(void)_scratch;
	}

	Thread* current_thread() const { return m_thread; }

	void set_thread(Thread* thread) { m_thread = thread; }

	uint8 current_ap() const { return m_ap_id; }

	void set_ap(uint8 const id) { m_ap_id = id; }

	Scheduler& scheduler() { return m_sched; }

	GDT* gdt() { return m_gdt; }

	void set_gdt(GDT* gdt) { m_gdt = gdt; }
} __attribute__((packed));
