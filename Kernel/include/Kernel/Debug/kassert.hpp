#pragma once

void __kassert_internal(const char*, int, const char*, bool);

#define TOSTRING(a) #a
#define kassert(a) __kassert_internal(__FILE__, __LINE__, TOSTRING(a), a)
#define assert(a) kassert(a)

#define ASSERT_IRQ_DISABLED() \
	{ \
        uint32_t flags;                         \
		asm volatile("pushf\n" "pop %0\n"       \
		: "=rm"(flags) :: "memory");            \
		if(flags & 0x0200u) __kassert_internal(__FILE__, __LINE__, \
							"Interrupts enabled, but expected them to be disabled!\n", \
							false); \
	}


#ifdef NDEBUG
#define assert(a)
#define kassert(a)
#define ASSERT_IRQ_DISABLED()
#endif
