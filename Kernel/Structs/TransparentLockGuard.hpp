#pragma once
#include <LibGeneric/Algorithm.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Memory.hpp>
#include <Structs/KFunction.hpp>

/*  Type-erased transparent lock guard class
 *
 *  This can be used to provide RAII-style lock acquisitions
 *  to users of a subsystem without having to expose the type
 *  of the internal lock structue.
 *  This uses KFunction to perform type erasure without having
 *  to allocate anything, so it is safe to use even during very
 *  early boot, when heap is not available yet.
 */
class TransparentLockGuard {
public:
	constexpr TransparentLockGuard() = delete;

	template<gen::Lock LockType>
	constexpr explicit TransparentLockGuard(LockType& lock) noexcept
	    : m_unlock_fn([&lock] { lock.unlock(); }) {
		lock.lock();
	}

	constexpr TransparentLockGuard(TransparentLockGuard&& lg) noexcept
	    : m_unlock_fn(gen::move(lg.m_unlock_fn)) {}

	constexpr TransparentLockGuard(TransparentLockGuard const&) = delete;

	constexpr ~TransparentLockGuard() noexcept { m_unlock_fn(); }
private:
	KFunction<void()> m_unlock_fn;
};
