section .text

global inFromPortB
inFromPortB:
	mov edx, [esp+4]
	in al, dx
	ret

global inFromPortD
inFromPortD:
	mov edx, [esp+4]
	in eax, dx
	ret

global outToPortB
outToPortB:
	mov edx, [esp+4]
	mov eax, [esp+8]
	out dx, al
	ret

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
