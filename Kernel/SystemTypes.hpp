#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

///  Common type definitions for the kernel

using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8 = int8_t;
using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8 = uint8_t;

constexpr uint64 operator"" _KiB(unsigned long long value) {
	return value * 0x400;
}

constexpr uint64 operator"" _MiB(unsigned long long value) {
	return value * 0x100000;
}

constexpr uint64 operator"" _GiB(unsigned long long value) {
	return value * 0x40000000;
}
