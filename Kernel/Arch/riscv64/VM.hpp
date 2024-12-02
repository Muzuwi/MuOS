#pragma once
#include <SystemTypes.hpp>

extern unsigned char __KERNEL_VM_BASE[];
extern unsigned char __KERNEL_VM_SIZE[];

#define _LINKSYM(a)             (reinterpret_cast<void*>(&a))
#define _PTRDIFF(a, b)          (static_cast<uintptr_t>((reinterpret_cast<uint8*>(a) - reinterpret_cast<uint8*>(b))))
#define _CONSTPTR(v)            (reinterpret_cast<void*>(v))
#define KERNEL_VM_IDENTITY_BASE _CONSTPTR(0xFFFFFFD000000000)
#define KERNEL_VM_IDENTITY_LEN  (0x1000000000)
#define KERNEL_VM_VMALLOC_BASE  _CONSTPTR(0xFFFFFFE000000000)
#define KERNEL_VM_VMALLOC_LEN   (0x1000000000)
#define KERNEL_VM_ELF_BASE      _LINKSYM(__KERNEL_VM_BASE)
#define KERNEL_VM_ELF_LEN       (reinterpret_cast<uintptr_t>(_LINKSYM(__KERNEL_VM_SIZE)))
#define KERNEL_VM_TEXT_BASE     _LINKSYM(__KERNEL_VM_BASE)
#define KERNEL_VM_TEXT_LEN      (reinterpret_cast<uintptr_t>(_LINKSYM(__KERNEL_VM_SIZE)))
#define KERNEL_PM_LOAD_BASE     _CONSTPTR(0x0000000080410000)
