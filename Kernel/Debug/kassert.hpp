#pragma once

void __kassert_internal(const char*, int, const char*, bool);

#define TOSTRING(a) #a
#define kassert(a) __kassert_internal(__FILE__, __LINE__, TOSTRING(a), a)
#define assert(a) kassert(a)

#define ASSERT_IRQ_DISABLED() \
	{ \
        uint64_t flags;                         \
		asm volatile("pushf\n" "pop %0\n"       \
		: "=rm"(flags) :: "memory");            \
		if(flags & 0x0200u) __kassert_internal(__FILE__, __LINE__, \
							"Interrupts enabled, but expected them to be disabled!\n", \
							false); \
	}

#define ASSERT_NONCONCURRENT() ASSERT_IRQ_DISABLED()

#define ASSERT_NOT_REACHED() \
	__kassert_internal(__FILE__, __LINE__, "Reached ASSERT_NOT_REACHED();", false)

#ifdef NDEBUG
#define assert(a)
#define kassert(a)
#define ASSERT_IRQ_DISABLED()
#endif
