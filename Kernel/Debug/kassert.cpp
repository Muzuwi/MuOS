#include <Debug/kassert.hpp>
#include <Debug/kpanic.hpp>
#include <Debug/klogf.hpp>

void __kassert_internal(const char* file, int line, const char* expr_str, bool expression) {
	if(!expression) {
		klogf_static("Kernel assertion failed: {}\n", expr_str);
		klogf_static("File: {}, line: {}\n", file, line);
		kpanic();
	}
}
