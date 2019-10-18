/*	
	Paging header
*/

#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct PD_ENTRY {
	uint32_t present : 1;
	uint32_t readwrite : 1;
	uint32_t supervisor : 1;
	uint32_t writethrough : 1;
	uint32_t cache : 1;
	uint32_t access : 1;
	uint32_t : 1;
	uint32_t pagesize : 1;
	uint32_t os_available: 3;
	uint32_t table_address : 20;
};

struct PT_ENTRY {
	uint32_t present : 1;
	uint32_t readwrite : 1;
	uint32_t supervisor : 1;
	uint32_t writethrough : 1;
	uint32_t cache : 1;
	uint32_t access : 1;
	uint32_t dirty : 1;
	uint32_t : 1;
	uint32_t global : 1;
	uint32_t os_available : 3;
	uint32_t page_address : 20;
};

struct PAGE_DIRECTORY {
	struct PD_ENTRY entries[1024];
};

struct PAGE_TABLE {
	struct PT_ENTRY pages[1024];
};

#ifdef __cplusplus
}
#endif

#endif