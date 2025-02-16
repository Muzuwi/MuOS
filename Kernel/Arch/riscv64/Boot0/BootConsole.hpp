#pragma once
#include <LibFDT/DeviceTree.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/Move.hpp>
#include <SystemTypes.hpp>

//  Prefix to append to most log lines
#define LOGPFX "[boot0] "

namespace bootcon {
	/**
	 *  init() and putch() must be implemented by the platform to
	 *  debug boot0.
	 */
	void init(libfdt::FdtHeader const*);
	void putch(char);
	void remap();

	template<typename... Args>
	inline static void printf(char const* format, Args... args) {
		static constexpr size_t BUFSIZE = 256;
		char buf[BUFSIZE];
		Format::format(format, buf, BUFSIZE, gen::forward<Args>(args)...);
		auto* ptr = buf;
		while(*ptr != '\0') {
			putch(*ptr);
			++ptr;
		}
	}
}
