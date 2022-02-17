#pragma once

#include <Debug/DebugCon.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Memory/KHeap.hpp>

class StaticKlogfBuffer {
public:
	static constexpr const size_t static_buffer_size = 256;
	char m_buffer[static_buffer_size] {};
	gen::Spinlock m_lock {};

	static StaticKlogfBuffer& instance() {
		static StaticKlogfBuffer s_buffer;
		return s_buffer;
	}
private:
	StaticKlogfBuffer() = default;
};

/*
 *  Logs a message to the kernel debugger.
 *  This function allocates a buffer on the heap, and uses it to format the message, which allows it
 *  to be thread and SMP-safe.
 */
template<typename... Args>
constexpr void klogf(char const* format, Args... args) {
	constexpr const size_t klogf_buffer_alloc_size = 256;
	auto buffer = KHeap::instance().slab_alloc(klogf_buffer_alloc_size);
	if(!buffer) {
		return;
	}
	Format::format(format, static_cast<char*>(buffer), klogf_buffer_alloc_size, args...);
	Debug::log_info(static_cast<char*>(buffer));
	KHeap::instance().slab_free(buffer, klogf_buffer_alloc_size);
}

/*
 *  Logs a message to the kernel debugger.
 *  As opposed to klogf, this function uses a static buffer which needs to be properly locked.
 *  This should only be used in contexts where allocations are impossible or would cause infinite recursion
 *  (i.e. KHeap implementations)
 */
template<typename... Args>
constexpr void klogf_static(char const* format, Args... args) {
	auto& buffer = StaticKlogfBuffer::instance();
	buffer.m_lock.lock();
	Format::format(format, buffer.m_buffer, StaticKlogfBuffer::static_buffer_size, args...);
	Debug::log_info(buffer.m_buffer);
	buffer.m_lock.unlock();
}

/*
 *  Logs an error message to the kernel debugger.
 */
template<typename... Args>
constexpr void kerrorf(char const* format, Args... args) {
	constexpr const size_t klogf_buffer_alloc_size = 256;
	auto buffer = KHeap::instance().slab_alloc(klogf_buffer_alloc_size);
	if(!buffer) {
		return;
	}
	Format::format(format, static_cast<char*>(buffer), klogf_buffer_alloc_size, args...);
	Debug::log_error(static_cast<char*>(buffer));
	KHeap::instance().slab_free(buffer, klogf_buffer_alloc_size);
}

/*
 *  Logs an error message to the kernel debugger (using a static buffer with locking for formatting the message)
 */
template<typename... Args>
constexpr void kerrorf_static(char const* format, Args... args) {
	auto& buffer = StaticKlogfBuffer::instance();
	buffer.m_lock.lock();
	Format::format(format, buffer.m_buffer, StaticKlogfBuffer::static_buffer_size, args...);
	Debug::log_error(buffer.m_buffer);
	buffer.m_lock.unlock();
}
