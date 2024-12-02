#pragma once
#include <SystemTypes.hpp>

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
	uint32 len;
	uint32 nameoff;
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

FdtHeader const* fdt_init_parser(uint8 const* fdtptr);
char const* fdt_node_name(FdtHeader const*, FdtNodeHandle nhandle);
char const* fdt_prop_name(FdtHeader const* header, FdtPropHandle phandle);
uint32 fdt_prop_len(FdtHeader const*, FdtPropHandle phandle);
char const* fdt_prop_read_string(FdtHeader const* header, FdtPropHandle phandle);
bool fdt_prop_read_u32(FdtHeader const* header, FdtPropHandle phandle, uint32* outptr, size_t cell = 0);
bool fdt_prop_read_u64(FdtHeader const* header, FdtPropHandle phandle, uint64* outptr, size_t cell = 0);

int fdt_find_all_nodes_by_unit_name(FdtHeader const* header, char const* unit_name, FdtNodeHandle* out,
                                    size_t out_capacity);
FdtNodeHandle fdt_find_first_node_by_unit_name(FdtHeader const* header, char const* unit_name);
bool fdt_find_next_child(FdtHeader const* header, FdtNodeHandle parent, FdtNodeHandle* out);
FdtPropHandle fdt_node_get_named_prop(FdtHeader const* header, FdtNodeHandle nhandle, char const* prop_name);

void fdt_visit_each_prop(FdtHeader const* header, FdtNodeHandle nhandle, FdtPropVisitor visitor, void* args);
void fdt_visit_each_node(FdtHeader const* header, FdtNodeVisitor visitor, void* args);