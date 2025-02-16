#pragma once
#include <stddef.h>
#include <stdint.h>

namespace libfdt {
	struct FdtHeader {
		uint32_t magic;
		uint32_t totalsize;
		uint32_t off_dt_struct;
		uint32_t off_dt_strings;
		uint32_t off_mem_rsvmap;
		uint32_t version;
		uint32_t last_comp_version;
		uint32_t boot_cpuid_phys;
		uint32_t size_dt_strings;
		uint32_t size_dt_struct;
	};

	struct FdtNode {
		uint32_t tag;
		uint32_t size;
		uint32_t data[0];//  Variable-length data
	};

	struct FdtProp {
		uint32_t len;
		uint32_t nameoff;
	};

	enum FdtToken {
		FDT_BEGIN_NODE = 0x00000001,
		FDT_END_NODE = 0x00000002,
		FDT_PROP = 0x00000003,
		FDT_NOP = 0x00000004,
		FDT_END = 0x00000009
	};

	using FdtNodeHandle = void*;
	using FdtPropHandle = void*;

	using FdtNodeVisitor = bool (*)(FdtHeader const*, FdtNodeHandle, void*);
	using FdtPropVisitor = bool (*)(FdtHeader const*, FdtNodeHandle, FdtPropHandle, void*);

	FdtHeader const* init_parser(uint8_t const* fdtptr);
	char const* node_name(FdtHeader const*, FdtNodeHandle nhandle);
	char const* prop_name(FdtHeader const* header, FdtPropHandle phandle);
	uint32_t prop_len(FdtHeader const*, FdtPropHandle phandle);
	char const* prop_read_string(FdtHeader const* header, FdtPropHandle phandle);
	bool prop_read_u32(FdtHeader const* header, FdtPropHandle phandle, uint32_t* outptr, size_t cell = 0);
	bool prop_read_u64(FdtHeader const* header, FdtPropHandle phandle, uint64_t* outptr, size_t cell = 0);

	int find_all_nodes_by_unit_name(FdtHeader const* header, char const* unit_name, FdtNodeHandle* out,
	                                size_t out_capacity);
	FdtNodeHandle find_first_node_by_unit_name(FdtHeader const* header, char const* unit_name);
	bool find_next_child(FdtHeader const* header, FdtNodeHandle parent, FdtNodeHandle* out);
	FdtPropHandle node_get_named_prop(FdtHeader const* header, FdtNodeHandle nhandle, char const* prop_name);

	void visit_each_prop(FdtHeader const* header, FdtNodeHandle nhandle, FdtPropVisitor visitor, void* args);
	void visit_each_node(FdtHeader const* header, FdtNodeVisitor visitor, void* args);
}
