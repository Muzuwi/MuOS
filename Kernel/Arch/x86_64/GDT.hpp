#pragma once

#include <Arch/x86_64/CPU.hpp>
#include <LibGeneric/Utility.hpp>
#include <SystemTypes.hpp>

#define TSS_RSP0 (4)
#define TSS_IOPB (0x66)

struct GDT {
public:
	struct TSS {
		uint8 data[104];
	} m_tss { create_tss() };

	struct GDTR {
		uint16 limit;
		uint64 base;
	} __attribute__((packed));
private:
	constexpr uint64 create_tss_higher_descriptor() { return (gen::bitcast<uint64>(&m_tss.data[0])) >> 32u; }

	constexpr uint64 create_tss_lower_descriptor() {
		const auto address = gen::bitcast<uint64>(&m_tss.data[0]);
		const auto base_h = (address & 0xFF000000) << 32u;
		const auto base_l = (address & 0x00FFFFFF) << 16u;
		const auto limit = sizeof(m_tss.data) & 0xFFFF;
		return base_h | 0x0000890000000000 | base_l | limit;
	}

	static constexpr TSS create_tss() {
		TSS tss {};
		const uint16 size = sizeof(tss) & 0x0000FFFF;
		tss.data[TSS_IOPB] = size & 0xFFu;
		tss.data[TSS_IOPB + 1] = size >> 8u;
		return tss;
	}

	constexpr GDTR create_gdtr() {
		GDTR gdtr {};
		gdtr.limit = sizeof(m_descriptor_table) - 1;
		gdtr.base = gen::bitcast<uint64>(&m_descriptor_table[0]);
		return gdtr;
	}

	static constexpr unsigned kernelcdescr_offset = 1, kernelddescr_offset = 2, usercdescr_offset = 5,
	                          userddescr_offset = 4, tss_offset = 6;

	static constexpr unsigned user_CS = usercdescr_offset * 8, user_DS = userddescr_offset * 8,
	                          kernel_CS = kernelcdescr_offset * 8, kernel_DS = kernelddescr_offset * 8,
	                          tss_sel = tss_offset * 8;
public:
	uint64 m_descriptor_table[8] {
		0x0,                           //  Null
		0x00209A0000000000,            //  Code0
		0x0000920000000000,            //  Data0
		0x0,                           //  Null
		0x0000F20000000000,            //  Data3
		0x0020FA0000000000,            //  Code3
		create_tss_lower_descriptor(), //  TSS_lower
		create_tss_higher_descriptor(),//  TSS_higher
	};

	GDTR m_descriptor { create_gdtr() };

	static constexpr unsigned get_user_CS() { return user_CS; }

	static constexpr unsigned get_user_DS() { return user_DS; }

	static constexpr unsigned get_kernel_CS() { return kernel_CS; }

	static constexpr unsigned get_kernel_DS() { return kernel_DS; }

	static constexpr unsigned get_user_base_seg() { return 3 * 8; }

	void set_irq_stack(void* stack) {
		*reinterpret_cast<uint64*>(&m_tss.data[TSS_RSP0]) = reinterpret_cast<uint64>(stack);
	}

	/**	Load the current GDT
	 */
	void load() {
		CPU::lgdt(&m_descriptor);
		CPU::ltr(GDT::tss_sel);
	}
};
