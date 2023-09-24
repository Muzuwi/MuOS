#pragma once

#include <Scheduler/Scheduler.hpp>
#include <SystemTypes.hpp>

class Thread;

struct GDT;

class ControlBlock {
	friend class SMP;
	friend struct GDT;

	//  Offset:
	ControlBlock* m_self_reference;//  0, store self reference for quick ctrlblock fetching
	Thread* m_thread;              //  8
	uint64 m_apic_id;              //  16
	uint64 _scratch;               //  24, only used in SysEntry to temporarily preserve user rsp
	//  -------------------------  //
	Scheduler m_sched;
	GDT* m_gdt;
	uint64 m_vid;
public:
	ControlBlock(uint8 ap_id) noexcept
	    : m_self_reference(this)
	    , m_thread(nullptr)
	    , m_apic_id(ap_id)
	    , _scratch(0)
	    , m_gdt(nullptr)
	    , m_vid(0) {
		(void)m_self_reference;
		(void)_scratch;
	}

	constexpr Thread* current_thread() const { return m_thread; }
	constexpr void set_thread(Thread* thread) { m_thread = thread; }

	constexpr uint8 apic_id() const { return m_apic_id; }
	constexpr Scheduler& scheduler() { return m_sched; }
	constexpr GDT* gdt() { return m_gdt; }
	constexpr uint64 vid() const { return m_vid; }
};
