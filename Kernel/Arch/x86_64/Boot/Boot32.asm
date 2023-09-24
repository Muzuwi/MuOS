%include "BootDefines.mac"

; This is the main entrypoint for the kernel, as GRUB jumps to the _entrypoint_x32 symbol.
; The 32-bit entrypoint provides a small single-page stack, initial GDT and null IDT
; After setting up long mode, it jumps to _entrypoint_x64, while passing along the multiboot
; info header. The callee is then responsible for setting up its' own stack and G/IDT

;  Multiboot Header definitions
ALIGNONPAGE equ 1 << 0
MEMINFO equ 1 << 1
MAGIC equ 0x1BADB002
FLAG equ ALIGNONPAGE | MEMINFO
CHECKSUM equ -(MAGIC + FLAG)

;  Multiboot header
section .multiboot
align 4
	dd MAGIC
	dd FLAG
	dd CHECKSUM
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0

;  32-bit bootstrap
section .bootstrap32
bits 32

extern _ukernel_elf_pages_needed
extern _ukernel_elf_pages_needed_in_bytes

global _entrypoint_x32
_entrypoint_x32:
    ;  Check if "valid" mbinfo
    test ebx, ebx
    jz .__dead

    ;  TODO: validate multiboot flags
    mov edx, ebx        ;  NOTE:  Save multiboot info table to edx
                        ;         until we set up a stack
    mov ecx, [ebx+44]   ;  = mmap_length
    mov esi, [ebx+48]   ;  *mmap_addr

    mov edi, esi
    add edi, ecx    ;  mmap_end
._mem_search:
    ;  Check type
    cmp dword [esi+MMAP_TYPE], 1
    jne ._next_entry

    ;  Available, check length
    ;  4K - temp stack
    ;  4*4K - paging structures
    ;  FIXME: will fail for length > 0xffffffff
    mov ebx, REQUIRED_MEM+_ukernel_elf_pages_needed_in_bytes
    cmp dword [esi+MMAP_LEN], ebx
    jl ._next_entry

    ;  Make sure required memory is addressable in 32-bit
    cmp dword [esi+8], 0
    jne ._next_entry
    mov eax, [esi+MMAP_BASE]
    add eax, REQUIRED_MEM
    cmp eax, [esi+MMAP_BASE]
    jl ._next_entry

    ;  Check alignment to 0x1000
    mov eax, [esi+MMAP_BASE]
    and eax, ~0xfff
    jnz ._next_entry

    ;  Success!
    mov esi, [esi+MMAP_BASE]
    jmp memory_setup

._next_entry:
    add esi, [esi]  ;  ptr += size
    add esi, 4      ;  ptr += 4
    cmp esi, edi    ;  while(ptr < mmap_end);
    jge .__dead
    jmp ._mem_search

.__dead:
    mov eax, 0xdeaddead
    cli
    hlt
    jmp .__dead

;  Set up memory, using buffer starting at ESI
memory_setup:
    ;  Alloc stack from free memory
    add esi, 0x1000
    mov esp, esi

    ;  Save multiboot table address - for higher kernel entrypoint
    push edx

    ;  Base page buffer address
    push esi

    mov ecx, (REQUIRED_PAGES-1)+_ukernel_elf_pages_needed   ;  Don't zero the stack, only the work buffers
._alloc_clear_loop:
    mov eax, esi
    call zero_page
    add esi, 0x1000
    dec ecx
    jnz ._alloc_clear_loop

    ;  Buffer start - EDI
    pop edi

    ;  Now set up paging, using the bootstrap buffer starting at EDI

    ;  PML4
    lea eax, [edi+BUF_PDPT] ;  Address of PDPT
    or eax, PAGE_PRESENT|PAGE_WRITE
    mov [edi+BUF_PML4], eax

    ;  PDPT
    lea eax, [edi+BUF_PD] ;  Address of PD
    or eax, PAGE_PRESENT|PAGE_WRITE
    mov [edi+BUF_PDPT], eax

    ;  PD
    lea eax, [edi+BUF_PT1] ;  Address of PT
    or eax, PAGE_PRESENT|PAGE_WRITE
    mov [edi+BUF_PD], eax

    push edi

    ;  Create page tables
    lea edi, [edi+BUF_PT1]
    mov eax, PAGE_PRESENT|PAGE_WRITE
._create_page_table:
    mov [edi], eax
    add eax, 0x1000
    add edi, 8
    cmp eax, 0x200000
    jb ._create_page_table

    mov edi, [esp]

    ;  Set up identity map of first 1 GiB
    lea edi, [edi+BUF_IDNT_PD]
    mov eax, PAGE_PRESENT|PAGE_WRITE|PAGE_BIG
._create_bootstrap_identity:
    mov [edi+0], eax
    add edi, 8
    add eax, 0x200000
    cmp eax, 0x40000000
    jb ._create_bootstrap_identity

    mov edi, [esp]

    lea eax, [edi+BUF_IDNT_PDPT]
    or eax, PAGE_PRESENT|PAGE_WRITE
    mov ecx, 256*8  ;  FIXME:  Hardcoded index for _ukernel_identity_start
    lea edi, [edi+BUF_PML4+ecx]
    mov [edi], eax

    mov edi, [esp]

    lea eax, [edi+BUF_IDNT_PD]
    or eax, PAGE_PRESENT|PAGE_WRITE
    mov ecx, 0*8    ;  FIXME:  Hardcoded index for _ukernel_identity_start
    lea edi, [edi+BUF_IDNT_PDPT+ecx]
    mov [edi], eax

    pop edi
    push edi    ;  Save the work buffer for the 64-bit entrypoint

    ;  Disable interrupts
    mov al, 0xff
    out 0xa1, al
    out 0x21, al
    nop
    nop
    cli

    ;  Set up IDT
    lea eax, [edi+BUF_IDT]
    call zero_page
    lidt [eax]

    ;  Enable PAE/PGE
    mov eax, CR4_PAE|CR4_PGE|CR4_PSE
    mov cr4, eax

    ;  Point cr3 to the PML4
    mov eax, edi
    mov cr3, eax

    ;  Enable long mode support in EFER
    mov ecx, 0xc0000080
    rdmsr
    or eax, EFER_LM  ;  TODO:  Crash when not supported
    wrmsr

    ;  Enable PE/PG
    mov eax, cr0
    or eax, CR0_PE|CR0_PG
    mov cr0, eax

    ;  Set up bootstrap GDT
    lea eax, [edi+BUF_GDT]   ;  GDT buffer offset + 0x4100
    ;  Null descriptor
    mov dword [eax], 0
    mov dword [eax+4], 0
    ;  Code
    mov dword [eax+8], 0
    mov dword [eax+12], 0x00209A00
    mov dword [eax+16], 0
    mov dword [eax+20], 0x00009200

    mov word [eax+24], 0
    mov word [eax+26], 24        ;  Size
    mov dword [eax+28], eax     ;  Base address

    lgdt [eax+26]

    pop edi     ;  Pass along the work buffer address to the 64-bit entrypoint
    pop ebx     ;  Restore multiboot table

    ;  Jump to the 64-bit entrypoint
extern _ukernel_physical_start
    jmp 0x08:_ukernel_physical_start

._lp:
    cli
    hlt
    jmp ._lp


;  Zero the page specified in eax
zero_page:
    push ecx    ;  No real ABI in here..

    ;  Ensure alignment
    and eax, ~0xfff
    mov ecx, 0x1000/4
._zero:
    mov dword [eax+ecx*4-4], 0
    loop ._zero

    pop ecx
    ret

