%include "Bootstage/BootDefines.mac"

;  This will be loaded at 0x00110000 by GRUB
;  64-bit entrypoint
extern _ukernel_elf_size
extern _ukernel_elf_start
extern _ukernel_elf_end
extern _ukernel_elf_pages_needed
extern _ukernel_physical_start

bits 64
section .entrypoint
_entrypoint_x64:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ;  At this point, rbx contains the multiboot table address,
    ;  and rdi the base of the work buffer used in the 32-bit loader

    ;  Save the multiboot address, we don't need it right now
    push rbx

    ;  Set up ELF mappings
    ;  PML4
    mov rax, _ukernel_elf_start
    ror rax, 39
    and rax, 0x1ff
    lea rdx, [rdi + BUF_PML4 + rax*8]
    mov rax, PAGE_PRESENT|PAGE_WRITE
    lea rcx, [rdi + BUF_ELF + BUF_ELF_PDPT]  ;  Address of PDPT in buffer
    add rax, rcx
    mov [rdx], rax

    ;  PDPT
    mov rax, _ukernel_elf_start
    ror rax, 30
    and rax, 0x1ff
    lea rdx, [rdi + BUF_ELF + BUF_ELF_PDPT + rax*8]
    mov rax, PAGE_PRESENT|PAGE_WRITE
    lea rcx, [rdi + BUF_ELF + BUF_ELF_PD] ; Address of PD in buffer
    add rax, rcx
    mov [rdx], rax

    ;  PD
    mov rax, _ukernel_elf_start ;  Current VMA
    mov rsi, 0  ;  Offset of the current PT in the work buffer

.map_pts:
    mov rbx, rax
    ror rbx, 21
    and rbx, 0x1ff
    lea rdx, [rdi + BUF_ELF + BUF_ELF_PD + rbx*8]
    lea rcx, [rdi + BUF_ELF + BUF_ELF_PT_BASE + rsi] ; Address of PT in buffer
    add rcx, PAGE_PRESENT|PAGE_WRITE
    mov [rdx], rcx
    add rax, 2*1024*1024    ;  +2MB
    add rsi, 0x1000
    mov rbx, _ukernel_elf_end
    cmp rax, rbx
    jb .map_pts

    ;  RDI now points to the PT base, as we don't need the other buffers anymore
    lea rdi, [rdi + BUF_ELF + BUF_ELF_PT_BASE]

    ;  PTs
    mov rbx, _ukernel_physical_start
    mov rcx, _ukernel_elf_start
.map_pages:
    mov rsi, rcx
    ror rsi, 12
    and rsi, 0x1ff
    lea rdx, [rdi + rsi*8]  ;  Points to PTE
    mov rax, PAGE_PRESENT|PAGE_WRITE
    add rax, rbx
    mov [rdx], rax
    add rbx, 0x1000
    add rcx, 0x1000
    ;  Check if we're at the PT boundary
    cmp rsi, 511
    jne ._skip
    lea rdi, [rdi + 0x1000] ;  Next PT
._skip:
    ;  Check if the entire executable has been mapped
    mov rdx, _ukernel_elf_end
    cmp rcx, rdx
    jb .map_pages

    mov rax, cr3
    mov cr3, rax

    ;  The ELF is now mapped, set up new stack
    pop rbx ;  Restore multiboot table from bootstrap stack before setting the new one
    mov rsp, __stack64

    ;  SysV - parameter 1 in rdi
    mov rdi, rbx

    mov rax, 0
    mov rbx, 0
    mov rcx, 0
    mov rdx, 0
    mov rsi, 0
    mov rbp, 0
    mov r8, 0
    mov r9, 0
    mov r10, 0
    mov r11, 0
    mov r12, 0
    mov r13, 0
    mov r14, 0
    mov r15, 0

    ;  Trampoline into kernel address space
    mov rax, high_trampoline
    push rax
    mov rax, 0
    ret

high_trampoline:
    ;  Preserve multiboot table address
    push rdi

    ;  Call constructors
	extern _init
	call _init

	;  Restore multiboot table address
	pop rdi

    ;  Jump to the actual kernel
    extern _ukernel_entrypoint
    call _ukernel_entrypoint

._lp64:
    cli
    hlt
    jmp ._lp64

;  Initialization stack
section .bss
    align 32
    resb 16384
__stack64:

;  Force generation of .rodata
section .rodata
    dd 0