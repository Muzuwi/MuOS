#pragma once
#include <stddef.h>
#include <stdint.h>
#include <asm/errno.h>
#include <sys/mman.h>

struct mem_range_t {
	uint64_t m_start, m_end;
	mem_range_t(uint64_t start, uint64_t end) {
		m_start = start;
		m_end = end;
	}
	mem_range_t(){
		m_start = 0;
		m_end = 0;
	}
	uint64_t size() {
		return m_end - m_start;
	}
};


typedef uint16_t allocation_t;


//  FIXME:  Placeholders
typedef int pid_t;
typedef int FSResult;
typedef uint64_t lba_t;
typedef char* path_t;