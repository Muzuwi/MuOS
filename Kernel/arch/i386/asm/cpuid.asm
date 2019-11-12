section .text

global cpu_get_msw
cpu_get_msw:
	smsw eax
	ret