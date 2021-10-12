#pragma once
#include <stddef.h>
#include <stdint.h>
#include <asm/errno.h>
#include <sys/types.h>

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;


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
typedef int FSResult;
typedef uint64_t lba_t;
typedef char* path_t;

typedef uint16_t umode_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t loff_t;
typedef uint32_t mode_t;
typedef int64_t ssize_t;
