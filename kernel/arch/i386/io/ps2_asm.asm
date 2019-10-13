section .text

;  Reading from the controller

global ps2ctrl_read
ps2ctrl_read:
	in ax, 0x60
	ret

global ps2ctrl_read_status
ps2ctrl_read_status:
	mov edi, [esp+4]
	in al, 0x64
	mov [edi], al
	ret

;  Writing to the controller

global ps2ctrl_write_command
ps2ctrl_write_command:
	mov eax, [esp+4]
	out 0x64, ax
	ret

global ps2ctrl_write_data
ps2ctrl_write_data:
	mov eax, [esp+4]
	out 0x60, ax
	ret


;  Misc routines

global ps2ctrl_flush
ps2ctrl_flush:
	in ax, 0x60
	in ax, 0x64
	test ax, 00000001b
	jnz ps2ctrl_flush	
	ret

global ps2ctrl_wait_input_empty
ps2ctrl_wait_input_empty:
	in al, 0x64
	test al, 00000010b 
	jnz ps2ctrl_wait_input_empty	
	ret

global ps2ctrl_wait_output_full
ps2ctrl_wait_output_full:
	in al, 0x64
	test al, 00000001b 
	jz ps2ctrl_wait_output_full
	ret
