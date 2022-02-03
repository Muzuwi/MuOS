#include <Debug/kassert.hpp>
#include <Debug/kpanic.hpp>
#include <Debug/klogf.hpp>

void __kassert_impl(const char* file, int line, const char* expr_str, bool expression) {
	if(!expression) {
		__kassert_panic(file, line, expr_str);
	}
}

void __kassert_panic(const char* file, int line, const char* expr_str) {
	klogf_static("Kernel assertion failed: {}\n", expr_str);
	klogf_static("File: {}, line: {}\n", file, line);
	kpanic();
}
