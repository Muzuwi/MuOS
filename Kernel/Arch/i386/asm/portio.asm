section .text

global loadGDT
loadGDT:
	mov eax, [esp+4]
	cli
	lgdt [eax]
	jmp 0x08:reloadSelectors
reloadSelectors:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	ret
