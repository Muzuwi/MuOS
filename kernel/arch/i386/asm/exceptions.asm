section .data
temp:
	dd 0

section .text

global isr_except_divbyzero
isr_except_divbyzero:
	pop eax
	mov [temp], eax
	push eax
	pusha
	
	push dword [temp]
	extern _kernel_exception_divbyzero
	call _kernel_exception_divbyzero
	pop eax

	popa
	iret

global isr_except_dbg
isr_except_dbg:
	pusha
	extern _kernel_exception_dbg
	call _kernel_exception_dbg
	popa
	iret

global isr_except_nmi
isr_except_nmi:
	pusha
	extern _kernel_exception_nmi
	call _kernel_exception_nmi
	popa
	iret

global isr_except_break
isr_except_break:
	pusha
	extern _kernel_exception_break
	call _kernel_exception_break
	popa
	iret

global isr_except_overflow
isr_except_overflow:
	pusha
	extern _kernel_exception_overflow
	call _kernel_exception_overflow
	popa
	iret

global isr_except_bound
isr_except_bound:
	pusha
	extern _kernel_exception_bound
	call _kernel_exception_bound
	popa
	iret

global isr_except_invalidop
isr_except_invalidop:
	pusha
	extern _kernel_exception_invalidop
	call _kernel_exception_invalidop
	popa
	iret

global isr_except_nodevice
isr_except_nodevice:
	pusha
	extern _kernel_exception_nodevice
	call _kernel_exception_nodevice
	popa
	iret

global isr_except_doublefault
isr_except_doublefault:
	pusha
	extern _kernel_exception_doublefault
	call _kernel_exception_doublefault
	popa
	iret

global isr_except_invalidtss
isr_except_invalidtss:
	pusha
	extern _kernel_exception_invalidtss
	call _kernel_exception_invalidtss
	popa
	iret

global isr_except_invalidseg
isr_except_invalidseg:
	pusha
	extern _kernel_exception_invalidseg
	call _kernel_exception_invalidseg
	popa
	iret

global isr_except_segstackfault
isr_except_segstackfault:
	pusha
	extern _kernel_exception_segstackfault
	call _kernel_exception_segstackfault
	popa
	iret

global isr_except_gpf
isr_except_gpf:
	pop eax
	mov [temp], eax
	push eax
	pusha

	push dword [temp]
	extern _kernel_exception_gpf
	call _kernel_exception_gpf
	pop eax

	popa
	iret

global isr_except_pagefault
isr_except_pagefault:
	pusha
	extern _kernel_exception_pagefault
	call _kernel_exception_pagefault
	popa
	iret

global isr_except_x87fpfault
isr_except_x87fpfault:
	pusha
	extern _kernel_exception_x87fpfault
	call _kernel_exception_x87fpfault
	popa
	iret

global isr_except_aligncheck
isr_except_aligncheck:
	pusha
	extern _kernel_exception_aligncheck
	call _kernel_exception_aligncheck
	popa
	iret

global isr_except_machinecheck
isr_except_machinecheck:
	pusha
	extern _kernel_exception_machinecheck
	call _kernel_exception_machinecheck
	popa
	iret

global isr_except_simdfp
isr_except_simdfp:
	pusha
	extern _kernel_exception_simdfp
	call _kernel_exception_simdfp
	popa
	iret

global isr_except_virtfault
isr_except_virtfault:
	pusha
	extern _kernel_exception_virtfault
	call _kernel_exception_virtfault
	popa
	iret

global isr_except_security
isr_except_security:
	pusha
	extern _kernel_exception_security
	call _kernel_exception_security
	popa
	iret
