#include <Arch/riscv64/Boot0/DeviceTree.hpp>
#include <SystemTypes.hpp>

static inline uint32_t be32toh(uint32_t bigEndianValue) {
	uint32_t result = ((bigEndianValue & 0xFF000000) >> 24) | ((bigEndianValue & 0x00FF0000) >> 8) |
	                  ((bigEndianValue & 0x0000FF00) << 8) | ((bigEndianValue & 0x000000FF) << 24);
	return result;
}

static inline uint64_t be64toh(uint64_t bigEndianValue) {
	uint64_t result =
	        ((bigEndianValue & 0xFF00000000000000ULL) >> 56) | ((bigEndianValue & 0x00FF000000000000ULL) >> 40) |
	        ((bigEndianValue & 0x0000FF0000000000ULL) >> 24) | ((bigEndianValue & 0x000000FF00000000ULL) >> 8) |
	        ((bigEndianValue & 0x00000000FF000000ULL) << 8) | ((bigEndianValue & 0x0000000000FF0000ULL) << 24) |
	        ((bigEndianValue & 0x000000000000FF00ULL) << 40) | ((bigEndianValue & 0x00000000000000FFULL) << 56);
	return result;
}

static int nonreloc_strncmp(const char* str1, const char* str2, size_t num) {
	unsigned int ptr = 0;
	do {
		if(str1[ptr] < str2[ptr])
			return -1;
		else if(str1[ptr] > str2[ptr])
			return 1;

		ptr++;
	} while(--num && str1[ptr - 1] != '\0' && str2[ptr - 1] != '\0');

	return 0;
}

static size_t nonreloc_strlen(const char* str) {
	size_t count = 0;
	while(str[count])
		count++;

	return count;
}

static int nonreloc_strcmp(const char* str1, const char* str2) {
	unsigned int ptr = 0;
	do {
		if(str1[ptr] < str2[ptr])
			return -1;
		else if(str1[ptr] > str2[ptr])
			return 1;

		ptr++;
	} while(str1[ptr - 1] != '\0' && str2[ptr - 1] != '\0');

	return 0;
}

void fdt_visit_each_node(FdtHeader const* header, FdtNodeVisitor visitor, void* args) {
	if(!header || !visitor) {
		return;
	}

	const uint32 off_dt_struct = be32toh(header->off_dt_struct);
	const uint32 totalsize = be32toh(header->totalsize);
	uint8 const* dt_struct = (uint8*)header + off_dt_struct;
	uint8 const* p = dt_struct;
	uint8 const* end = (uint8 const*)header + totalsize;
	while(p < end) {
		uint32_t token = be32toh(*reinterpret_cast<const uint32_t*>(p));
		p += 4;

		if(token == FDT_BEGIN_NODE) {
			auto* nhandle = (FdtNodeHandle*)p;
			if(!visitor(header, nhandle, args)) {
				break;
			}
			const char* name = reinterpret_cast<const char*>(p);
			p += nonreloc_strlen(name) + 1;
			p = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);//  Align to 4 bytes
		} else if(token == FDT_END_NODE) {
			//  End of a node, just continue
		} else if(token == FDT_PROP) {
			uint32_t len = be32toh(*reinterpret_cast<const uint32_t*>(p));
			//  len + nameoff
			p += 8;
			p += len;
			p = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);//  Align to 4 bytes
		} else if(token == FDT_NOP) {
			//  No operation, just continue
		} else if(token == FDT_END) {
			//  End of the structure block
			break;
		} else {
			break;
		}
	}
}

void fdt_visit_each_prop(FdtHeader const* header, FdtNodeHandle nhandle, FdtPropVisitor visitor, void* args) {
	if(!header || !nhandle || !visitor) {
		return;
	}

	const uint32 off_dt_struct = be32toh(header->off_dt_struct);
	const uint32 totalsize = be32toh(header->totalsize);
	uint8 const* dt_struct = (uint8*)header + off_dt_struct;
	uint8 const* p = dt_struct;
	uint8 const* end = (uint8 const*)header + totalsize;
	bool in_node = false;

	while(p < end) {
		uint32_t token = be32toh(*reinterpret_cast<const uint32_t*>(p));
		p += 4;

		if(token == FDT_BEGIN_NODE) {
			if(p == nhandle) {
				in_node = true;
			}

			const char* name = reinterpret_cast<const char*>(p);
			p += nonreloc_strlen(name) + 1;
			p = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);//  Align to 4 bytes
		} else if(token == FDT_END_NODE) {
			in_node = false;
			//  End of a node, just continue
		} else if(token == FDT_PROP) {
			auto* phandle = (FdtPropHandle*)p;
			if(in_node) {
				if(!visitor(header, nhandle, phandle, args)) {
					break;
				}
			}
			uint32_t len = be32toh(*reinterpret_cast<const uint32_t*>(p));
			//  len + nameoff
			p += 8;
			p += len;
			p = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);//  Align to 4 bytes
		} else if(token == FDT_NOP) {
			//  No operation, just continue
		} else if(token == FDT_END) {
			//  End of the structure block
			break;
		} else {
			break;
		}
	}
}

static inline void* _fdt_prop_data_ptr(FdtHeader const*, FdtPropHandle phandle) {
	return (void*)((char const*)phandle + 8);
}

FdtHeader const* fdt_init_parser(uint8 const* fdtptr) {
	if(!fdtptr) {
		return nullptr;
	}

	const uint32_t FDT_MAGIC = 0xD00DFEED;
	auto const* header = reinterpret_cast<FdtHeader const*>(fdtptr);

	if(!header) {
		return nullptr;
	}

	if(header->magic != be32toh(FDT_MAGIC)) {
		return nullptr;
	}

	return header;
}

/*
 *	Property fetchers
 */

char const* fdt_node_name(FdtHeader const* header, FdtNodeHandle nhandle) {
	if(!header || !nhandle) {
		return nullptr;
	}
	return static_cast<char const*>(nhandle);
}

char const* fdt_prop_name(FdtHeader const* header, FdtPropHandle phandle) {
	if(!header || !phandle) {
		return nullptr;
	}
	const uint32 nameoff = be32toh(*(static_cast<uint32*>(phandle) + 1));
	const uint32 off_dt_strings = be32toh(header->off_dt_strings);
	char const* dt_strings = (char const*)header + off_dt_strings;
	return dt_strings + nameoff;
}

uint32 fdt_prop_len(FdtHeader const* header, FdtPropHandle phandle) {
	if(!header || !phandle) {
		return 0;
	}
	const uint32 len = be32toh(*(static_cast<uint32*>(phandle) + 0));
	return len;
}

char const* fdt_prop_read_string(FdtHeader const* header, FdtPropHandle phandle) {
	if(!header || !phandle) {
		return nullptr;
	}
	if(fdt_prop_len(header, phandle) == 0) {
		return nullptr;
	}
	return (char const*)_fdt_prop_data_ptr(header, phandle);
}

bool fdt_prop_read_u32(FdtHeader const* header, FdtPropHandle phandle, uint32* outptr, size_t cell) {
	if(!header || !phandle || !outptr) {
		return false;
	}
	if(fdt_prop_len(header, phandle) < 4 * (cell + 1)) {
		return false;
	}

	uint32 value;
	__builtin_memcpy(&value, _fdt_prop_data_ptr(header, phandle), 4);
	*outptr = be32toh(value);
	return true;
}

bool fdt_prop_read_u64(FdtHeader const* header, FdtPropHandle phandle, uint64* outptr, size_t cell) {
	if(!header || !phandle || !outptr) {
		return false;
	}
	if(fdt_prop_len(header, phandle) < 8 * (cell + 1)) {
		return false;
	}

	uint64 value;
	__builtin_memcpy(&value, (char const*)_fdt_prop_data_ptr(header, phandle) + cell * 8, 8);
	*outptr = be64toh(value);
	return true;
}

struct _FdtFindContext {
	char const* name;
	void* handle;
	size_t max_handles;
	size_t found_count;
};

static bool _fdt_node_cb(FdtHeader const* header, FdtNodeHandle nhandle, void* args) {
	auto* ctx = static_cast<_FdtFindContext*>(args);
	auto* name = fdt_node_name(header, nhandle);

	const auto name_len = nonreloc_strlen(name);
	const auto unit_name_len = nonreloc_strlen(ctx->name);
	//  Check by comparing the prefix of the name string
	if(nonreloc_strncmp(ctx->name, name, unit_name_len) != 0) {
		return true;
	}
	//  The unit name we're searching of is a prefix of the current node's unit name

	//  Check the character following the unit name prefix to handle unit addresses;
	//	- if it is not a @, then the unit name does not match
	//	- if it is a @, we found our node
	//	For the name_len == unit_name_len case (when we do not have a unit address),
	//  this if statement is ignored.
	if(name_len > unit_name_len && name[unit_name_len] != '@') {
		return true;
	}

	auto* ptr = static_cast<FdtNodeHandle*>(ctx->handle);
	if(ctx->found_count < ctx->max_handles) {
		ptr[ctx->found_count] = nhandle;
	}
	++(ctx->found_count);
	return false;
}

static bool _fdt_prop_cb(FdtHeader const* header, FdtNodeHandle, FdtPropHandle phandle, void* args) {
	auto* ctx = static_cast<_FdtFindContext*>(args);
	auto* name = fdt_prop_name(header, phandle);

	if(nonreloc_strcmp(ctx->name, name) != 0) {
		return true;
	}

	auto* ptr = static_cast<FdtPropHandle*>(ctx->handle);
	if(ctx->found_count < ctx->max_handles) {
		ptr[ctx->found_count] = phandle;
	}
	++ctx->found_count;
	return false;
}

int fdt_find_all_nodes_by_unit_name(FdtHeader const* header, char const* unit_name, FdtNodeHandle* out,
                                    size_t out_capacity) {
	if(!header || !unit_name || !out) {
		return 0;
	}
	if(out_capacity == 0) {
		return 0;
	}

	_FdtFindContext ctx {};
	ctx.name = unit_name;
	ctx.handle = out;
	ctx.max_handles = out_capacity;
	fdt_visit_each_node(header, _fdt_node_cb, &ctx);
	return ctx.found_count;
}

FdtNodeHandle fdt_find_first_node_by_unit_name(FdtHeader const* header, char const* unit_name) {
	if(!header || !unit_name) {
		return nullptr;
	}

	FdtNodeHandle out = nullptr;
	_FdtFindContext ctx {};
	ctx.name = unit_name;
	ctx.handle = &out;
	ctx.max_handles = 1;
	fdt_visit_each_node(header, _fdt_node_cb, &ctx);
	return out;
}

FdtPropHandle fdt_node_get_named_prop(FdtHeader const* header, FdtNodeHandle nhandle, char const* prop_name) {
	if(!header || !nhandle || !prop_name) {
		return nullptr;
	}

	FdtPropHandle out = nullptr;
	_FdtFindContext ctx {};
	ctx.name = prop_name;
	ctx.handle = &out;
	ctx.max_handles = 1;
	fdt_visit_each_prop(header, nhandle, _fdt_prop_cb, &ctx);
	return out;
}

bool fdt_find_next_child(FdtHeader const* header, FdtNodeHandle parent, FdtNodeHandle* out) {
	if(!header || !parent || !out) {
		return false;
	}

	const FdtNodeHandle initial_node = *out;
	const uint32 totalsize = be32toh(header->totalsize);
	uint8 const* end = (uint8 const*)header + totalsize;

	size_t depth = 0;

	//  If we have an initial node handle, use that.
	//  Otherwise, skip to the first child token from the parent. `p` may not be
	//  pointing to a FDT_BEGIN_NODE token, but it doesn't matter as the logic
	//  remains the same.
	uint8* p;
	if(initial_node) {
		p = static_cast<uint8*>(initial_node);
		//  Set depth=1 to skip over the child node
		depth = 1;
	} else {
		p = static_cast<uint8*>(parent);
		p += nonreloc_strlen(reinterpret_cast<char const*>(p)) + 1;
		p = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);//  Align to 4 bytes
	}

	while(p < end) {
		uint32_t token = be32toh(*reinterpret_cast<uint32_t const*>(p));
		p += 4;

		if(token == FDT_BEGIN_NODE) {
			//  This is a direct child node from the parent
			if(depth == 0) {
				*out = reinterpret_cast<FdtNodeHandle>(p);
				return true;
			}
			++depth;

			const char* name = reinterpret_cast<char const*>(p);
			p += nonreloc_strlen(name) + 1;
			p = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);//  Align to 4 bytes
		} else if(token == FDT_END_NODE) {
			//  No more nodes
			if(depth == 0) {
				*out = nullptr;
				return false;
			}
			--depth;
		} else if(token == FDT_PROP) {
			uint32_t len = be32toh(*reinterpret_cast<uint32_t const*>(p));
			//  len + nameoff
			p += 8;
			p += len;
			p = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(p) + 3) & ~3);//  Align to 4 bytes
		} else if(token == FDT_END) {
			//  End of the structure block
			*out = nullptr;
			return false;
		}
	}

	return false;
}

//  TODO: fdt_find_root