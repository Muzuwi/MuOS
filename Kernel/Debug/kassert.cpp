#include <Kernel/Debug/kassert.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Debug/kdebugf.hpp>

void __kassert_internal(const char* file, int line, const char* expr_str, bool expression) {
	if(!expression) {
		kerrorf("Kernel assertion failed: %s\n", expr_str);
		kerrorf("File: %s, line: %i\n", file, line);
		kpanic();
	}
}
