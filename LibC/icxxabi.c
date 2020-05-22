#include <icxxabi.h>

#ifdef __cplusplus
extern "C"
#endif

extern void* __dso_handle;

struct atexit_func_entry_t __atexit_funcs[ATEXIT_MAX_FUNCS];
uarch_t __atexit_func_cnt = 0;

void __cxa_pure_virtual() {

}

int __cxa_atexit(void (*dtor)(void*), void *objptr, void *dso_handle) {
	if(__atexit_func_cnt >= ATEXIT_MAX_FUNCS) return -1;

	__atexit_funcs[__atexit_func_cnt].objptr = objptr;
	__atexit_funcs[__atexit_func_cnt].dtor = dtor;
	__atexit_funcs[__atexit_func_cnt].dso_handle = dso_handle;
	__atexit_func_cnt++;
	return 0;
}

void __cxa_finalize(void *dtor) {
	uarch_t i = __atexit_func_cnt;
	if(!dtor) {
		while(i--) {
			if(__atexit_funcs[i].dtor) {
				(*__atexit_funcs[i].dtor)(__atexit_funcs[i].objptr);
			}
		}
		return;
	}

	while(i--) {
		if(__atexit_funcs[i].dtor == dtor) {
			(*__atexit_funcs[i].dtor)(__atexit_funcs[i].objptr);
			//  TODO: Move all upper entries when removing
			__atexit_funcs[i].dtor = 0;
		}
	}
}


#ifdef __cplusplus
}
#endif
