#pragma once

void __kassert_internal(const char*, int, const char*, bool);

#define TOSTRING(a) #a
#define kassert(a) __kassert_internal(__FILE__, __LINE__, TOSTRING(a), a)
#define assert(a) kassert(a)

#ifdef NDEBUG
#define assert(a)
#define kassert(a)
#endif
