section .text

global loadIDT
loadIDT:
	mov edx, [esp+4]
	cli
	lidt [edx]
	sti
	ret

global irq0
irq0:
	pusha
	extern _kernel_irq0_handler
	call _kernel_irq0_handler
	popa
	iret

global irq1
irq1:
	pusha
	extern _kernel_irq1_handler
	call _kernel_irq1_handler
	popa
	iret
global irq2
irq2:	
	pusha
	extern _kernel_irq2_handler
	call _kernel_irq2_handler
	popa
	iret
global irq3
irq3:	
	pusha
	extern _kernel_irq3_handler
	call _kernel_irq3_handler
	popa
	iret
global irq4
irq4:	
	pusha
	extern _kernel_irq4_handler
	call _kernel_irq4_handler
	popa
	iret
global irq5
irq5:	
	pusha
	extern _kernel_irq5_handler
	call _kernel_irq5_handler
	popa
	iret
global irq6
irq6:	
	pusha
	extern _kernel_irq6_handler
	call _kernel_irq6_handler
	popa
	iret
global irq7
irq7:	
	pusha
	extern _kernel_irq7_handler
	call _kernel_irq7_handler
	popa
	iret
global irq8
irq8:	
	pusha
	extern _kernel_irq8_handler
	call _kernel_irq8_handler
	popa
	iret
global irq9
irq9:	
	pusha
	extern _kernel_irq9_handler
	call _kernel_irq9_handler
	popa
	iret
global irq10
irq10:	
	pusha
	extern _kernel_irq10_handler
	call _kernel_irq10_handler
	popa
	iret
global irq11
irq11:	
	pusha
	extern _kernel_irq11_handler
	call _kernel_irq11_handler
	popa
	iret
global irq12
irq12:	
	pusha
	extern _kernel_irq12_handler
	call _kernel_irq12_handler
	popa
	iret
global irq13
irq13:	
	pusha
	extern _kernel_irq13_handler
	call _kernel_irq13_handler
	popa
	iret
global irq14
irq14:	
	pusha
	extern _kernel_irq14_handler
	call _kernel_irq14_handler
	popa
	iret
global irq15
irq15:	
	pusha
	extern _kernel_irq15_handler
	call _kernel_irq15_handler
	popa
	iret

