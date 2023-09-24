#pragma once

[[noreturn]] void __kassert_panic(const char*, int, const char*);

void __kassert_impl(const char*, int, const char*, bool);

#define TOSTRING(a) #a
#define kassert(a)  __kassert_impl(__FILE__, __LINE__, TOSTRING(a), a)
#define assert(a)   kassert(a)

#define ASSERT_NOT_REACHED() __kassert_panic(__FILE__, __LINE__, "Reached ASSERT_NOT_REACHED()")
