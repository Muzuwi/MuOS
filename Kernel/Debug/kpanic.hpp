#pragma once

[[noreturn]] void _kpanic_internal(const char*, int);
#define kpanic() _kpanic_internal(__PRETTY_FUNCTION__, __LINE__)
