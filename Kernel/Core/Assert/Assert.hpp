#pragma once
#include <Core/Assert/Panic.hpp>

//  Helpers for macro stringification
#define MACRO_STR(x)      MACRO_STR_IMPL(x)
#define MACRO_STR_IMPL(x) #x

//  Ensure at runtime that a given expression evaluates to true
//  If the expression is false instead, a kernel panic is triggered.
//  ENSURE statements are not optimized out from debug builds, so they
//  can be used for sanity checks in critical places.
#define ENSURE(expression)                                                               \
	do {                                                                                 \
		if(!static_cast<bool>(expression)) {                                             \
			core::panic("ENSURE failed: Expression " #expression " evaluated to false"); \
		}                                                                                \
	} while(false)

//  Assert at runtime that a given expression evaluates to true.
//  If the expression is false instead, a kernel panic is triggered.
//  The check is removed from non-debug builds. Use ENSURE for
//  checks that should persist across all build types.
#define DEBUG_ASSERT(expression)                                                         \
	do {                                                                                 \
		if(!static_cast<bool>(expression)) {                                             \
			core::panic("ASSERT failed: Expression " #expression " evaluated to false"); \
		}                                                                                \
	} while(false)

//  Check the given expression at runtime, and trigger a warning if
//  it evaluates to true. A message can be given to customize the warning.
#define WARN_ON(expression, message)                                              \
	do {                                                                          \
		if(static_cast<bool>(expression)) {                                       \
			core::warn("Expression " #expression " evaluated to true: " message); \
		}                                                                         \
	} while(false)

///  *_NOT_REACHED

//  Assert at runtime that execution never reaches this statement.
//  If the execution reaches the statement, a kernel panic is triggered.
//  Similar to DEBUG_ASSERT, this check is removed from debug builds.
#define DEBUG_ASSERT_NOT_REACHED()                                                                  \
	do {                                                                                            \
		core::panic("Reached DEBUG_ASSERT_NOT_REACHED in file: " __FILE__ ":" MACRO_STR(__LINE__)); \
	} while(false)

//  Ensure that execution never reaches this statement.
//  If the execution reaches the statement, a kernel panic is triggered.
//  This check is NOT removed from debug builds.
#define ENSURE_NOT_REACHED()                                                                  \
	do {                                                                                      \
		core::panic("Reached ENSURE_NOT_REACHED in file: " __FILE__ ":" MACRO_STR(__LINE__)); \
	} while(false)

//  Hang the current CPU in an infinite loop
#define HANG() \
	do {       \
	} while(true)

#ifdef NDEBUG

#	undef DEBUG_ASSERT
#	define DEBUG_ASSERT(expression) \
		do {                         \
		} while(false)

#	undef DEBUG_ASSERT_NOT_REACHED
#	define DEBUG_ASSERT_NOT_REACHED() \
		do {                           \
		} while(false)

#endif
