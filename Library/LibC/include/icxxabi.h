#ifndef _ICXXABI_H
#define _ICXXABI_H

#define ATEXIT_MAX_FUNCS 128

#ifdef __cplusplus
extern "C" {
#endif

	typedef unsigned uarch_t;

	struct atexit_func_entry_t {
		void (*dtor)(void*);
		void* objptr;
		void* dso_handle;
	};

	int __cxa_atexit(void (*f)(void*), void* obj, void* dso);
	void __cxa_finalize(void* f);

#ifdef __cplusplus
}
#endif

#endif
