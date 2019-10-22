section .data
temp:
	dd 0

reg_store:
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0



%macro save_regs 0
	mov [reg_store + 0], eax
	mov [reg_store + 4], ebx 
	mov [reg_store + 8], ecx 
	mov [reg_store + 12], edx
	mov [reg_store + 16], esp 
	mov [reg_store + 20], ebp 
	mov [reg_store + 24], edi 
	mov [reg_store + 28], esi
	pop eax 
	mov [reg_store + 32], eax
	push eax
%endmacro


section .text

global isr_except_divbyzero
isr_except_divbyzero:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_divbyzero
	call _kernel_exception_divbyzero
	
	pop eax
	popa
	iret

global isr_except_dbg
isr_except_dbg:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_dbg
	call _kernel_exception_dbg
	
	pop eax
	popa
	iret

global isr_except_nmi
isr_except_nmi:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_nmi
	call _kernel_exception_nmi
	
	pop eax
	popa
	iret

global isr_except_break
isr_except_break:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_break
	call _kernel_exception_break
	
	pop eax
	popa
	iret

global isr_except_overflow
isr_except_overflow:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_overflow
	call _kernel_exception_overflow
	
	pop eax
	popa
	iret

global isr_except_bound
isr_except_bound:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_bound
	call _kernel_exception_bound
	
	pop eax
	popa
	iret

global isr_except_invalidop
isr_except_invalidop:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_invalidop
	call _kernel_exception_invalidop
	
	pop eax
	popa
	iret

global isr_except_nodevice
isr_except_nodevice:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_nodevice
	call _kernel_exception_nodevice
	
	pop eax
	popa
	iret

global isr_except_doublefault
isr_except_doublefault:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_doublefault
	call _kernel_exception_doublefault
	
	pop eax
	popa
	iret

global isr_except_invalidtss
isr_except_invalidtss:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_invalidtss
	call _kernel_exception_invalidtss
	
	pop eax
	popa
	iret

global isr_except_invalidseg
isr_except_invalidseg:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_invalidseg
	call _kernel_exception_invalidseg
	
	pop eax
	popa
	iret

global isr_except_segstackfault
isr_except_segstackfault:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_segstackfault
	call _kernel_exception_segstackfault
	
	pop eax
	popa
	iret

global isr_except_gpf
isr_except_gpf:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_gpf
	call _kernel_exception_gpf
	
	pop eax
	popa
	iret

global isr_except_pagefault
isr_except_pagefault:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_pagefault
	call _kernel_exception_pagefault
	
	pop eax
	popa
	iret

global isr_except_x87fpfault
isr_except_x87fpfault:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_x87fpfault
	call _kernel_exception_x87fpfault
	
	pop eax
	popa
	iret

global isr_except_aligncheck
isr_except_aligncheck:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_aligncheck
	call _kernel_exception_aligncheck
	
	pop eax
	popa
	iret

global isr_except_machinecheck
isr_except_machinecheck:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_machinecheck
	call _kernel_exception_machinecheck
	
	pop eax
	popa
	iret

global isr_except_simdfp
isr_except_simdfp:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_simdfp
	call _kernel_exception_simdfp
	
	pop eax
	popa
	iret

global isr_except_virtfault
isr_except_virtfault:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_virtfault
	call _kernel_exception_virtfault
	
	pop eax
	popa
	iret

global isr_except_security
isr_except_security:
	save_regs
	pusha
	push dword reg_store

	extern _kernel_exception_security
	call _kernel_exception_security
	
	pop eax
	popa
	iret
