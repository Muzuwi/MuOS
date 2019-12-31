#pragma once

[[noreturn]] void _kpanic_internal(const char*, int);
#define kpanic() _kpanic_internal(__FILE__, __LINE__)
