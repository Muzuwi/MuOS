#include <Core/Log/Logger.hpp>
#include <Debug/kassert.hpp>
#include <Debug/kpanic.hpp>

CREATE_LOGGER("assert", core::log::LogLevel::Debug);

void __kassert_impl(const char* file, int line, const char* expr_str, bool expression) {
	if(!expression) {
		__kassert_panic(file, line, expr_str);
	}
}

void __kassert_panic(const char* file, int line, const char* expr_str) {
	log.fatal("Kernel assertion failed: {}", expr_str);
	log.fatal("File: {}, line: {}", file, line);
	kpanic();
}
