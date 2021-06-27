#pragma once
#include <Kernel/SystemTypes.hpp>

class Process;

class ControlBlock {
	//                              Offset
	ControlBlock& m_self_reference; //  0, store self reference for quick ctrlblock fetching
	Process* m_process;             //  8
	uint64   m_ap_id;               //  16
	uint64   _scratch;        //  24 - only used in SysEntry to temporarily preserve user rsp
public:
	ControlBlock(uint8 ap_id) noexcept
	: m_self_reference(*this), m_process(nullptr), m_ap_id(ap_id), _scratch(0) {
		(void)m_self_reference;
		(void)_scratch;
	}

	Process* current_process() const {
		return m_process;
	}

	void set_process(Process* process) {
		m_process = process;
	}

	uint8 current_ap() const {
		return m_ap_id;
	}

} __attribute__((packed));
