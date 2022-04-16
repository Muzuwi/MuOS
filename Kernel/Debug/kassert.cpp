#include <Debug/kassert.hpp>
#include <Debug/klogf.hpp>
#include <Debug/kpanic.hpp>

void __kassert_impl(const char* file, int line, const char* expr_str, bool expression) {
	if(!expression) {
		__kassert_panic(file, line, expr_str);
	}
}

void __kassert_panic(const char* file, int line, const char* expr_str) {
	kerrorf_static("Kernel assertion failed: {}\n", expr_str);
	kerrorf_static("File: {}, line: {}\n", file, line);
	kpanic();
}
