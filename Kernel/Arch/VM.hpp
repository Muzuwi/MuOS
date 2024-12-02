#pragma once
#include <Core/Error/Error.hpp>
#include <LibGeneric/BitFlags.hpp>

/*  Kernel virtual memory configuration
 */

/* Architecture includes go here */
#ifdef ARCH_IS_x86_64
#	include <Arch/x86_64/VM.hpp>
#endif
#ifdef ARCH_IS_RISCV64
#	include <Arch/riscv64/VM.hpp>
#endif

#ifndef KERNEL_VM_IDENTITY_BASE
#	error "Missing architecture definition: KERNEL_VM_IDENTITY_BASE (Base address of the identity region in virtual memory)"
#endif
#ifndef KERNEL_VM_IDENTITY_LEN
#	error "Missing architecture definition: KERNEL_VM_IDENTITY_LEN (Length of the identity region in virtual memory)"
#endif
#ifndef KERNEL_VM_VMALLOC_BASE
#	error "Missing architecture definition: KERNEL_VM_VMALLOC_BASE (Base address of the vmalloc region in virtual memory)"
#endif
#ifndef KERNEL_VM_VMALLOC_LEN
#	error "Missing architecture definition: KERNEL_VM_VMALLOC_LEN (Length of the vmalloc region in virtual memory)"
#endif
#ifndef KERNEL_VM_ELF_BASE
#	error "Missing architecture definition: KERNEL_VM_ELF_BASE (Base address of the kernel ELF in virtual memory)"
#endif
#ifndef KERNEL_VM_ELF_LEN
#	error "Missing architecture definition: KERNEL_VM_ELF_LEN (Length of the kernel ELF in virtual memory)"
#endif
#ifndef KERNEL_VM_TEXT_BASE
#	error "Missing architecture definition: KERNEL_VM_TEXT_BASE (Base address of kernel executable/code sections in virtual memory)"
#endif
#ifndef KERNEL_VM_TEXT_LEN
#	error "Missing architecture definition: KERNEL_VM_TEXT_LEN (Length of the kernel executable/code sections in virtual memory)"
#endif

#ifndef KERNEL_PM_LOAD_BASE
#	error "Missing architecture definition: KERNEL_PM_LOAD_BASE (Physical address the kernel is loaded by the bootloader)"
#endif

/* Custom helpers go here */
#ifndef idmap
/* idmap - turn a given physical address to a pointer in the physical identity memory map space */
#	define idmap(physptr)                                                                                   \
		((physptr < KERNEL_VM_IDENTITY_BASE)                                                                 \
		         ? reinterpret_cast<decltype(physptr)>(reinterpret_cast<uint8_t*>(KERNEL_VM_IDENTITY_BASE) + \
		                                               reinterpret_cast<uintptr_t>(physptr))                 \
		         : physptr)
#endif

#ifndef idunmap
/* idunmap - turn a given identity map pointer back to a physical address */
#	define idunmap(idptr)                                                                                 \
		((idptr >= KERNEL_VM_IDENTITY_BASE)                                                                \
		         ? reinterpret_cast<decltype(idptr)>(reinterpret_cast<uint8_t*>(idptr) -                   \
		                                             reinterpret_cast<uintptr_t>(KERNEL_VM_IDENTITY_BASE)) \
		         : idptr)
#endif

namespace arch {
	enum class PageFlags {
		///  Page is readable
		Read = 1U << 1U,
		///  Page is writable
		Write = 1U << 2U,
		///  Page is executable
		Execute = 1U << 3U,
		///  Page should be accessible in whatever the architecture considers user mode
		User = 1U << 4U,
		///  Page should be considered a large page
		///  The architecture defines what it considers a large page, and further
		///  alignment constraints will be placed on the physical address used for
		///  mappings. Large pages may not be supported everywhere.
		Large = 1U << 5U,
		///  Page should be considered a huge page
		///  The architecture defines what it considers a huge page, and further
		///  alignment constraints will be placed on the physical address used for
		///  mappings. Huge pages may not be supported everywhere.
		Huge = 1U << 6U
	};
	DEFINE_ENUM_BITFLAG_OPS(PageFlags);

	using PagingHandle = void*;

	///  Allocate the top-level paging structure
	///  This allocates a platform-specific paging structure and returns a pointer to it.
	///	 This pointer will usually point to physical memory and must not be used directly.
	core::Result<PagingHandle> addralloc();
	///  RECURSIVELY free the given top-level paging structure
	///  This will free pages allocated on ALL levels of the structure
	core::Error addrfree(PagingHandle);
	///  Clone a given paging structure
	///  This allocates a new top-level paging structure and clones all existing mappings
	///  from the source structure. Further modifications to the paging structure will
	///  only affect the clone, not the original mappings (with exception to the kernel
	///  ranges which must be shared across all paging structures).
	core::Result<PagingHandle> addrclone(PagingHandle root);

	///  Create a mapping between virt <-> phys in the given paging structure
	core::Error addrmap(PagingHandle, void* pptr, void* vptr, PageFlags flags);
	///  Unmap a given virtual address
	core::Error addrunmap(PagingHandle, void* vptr);

	///  Physical pointer container class
	///  This can be used to distinguish between virtual and physical pointers
	///  and to avoid bugs related to the confusion of the two. This also provides
	///  utilities to easily access the pointer by internally calling `idmap` when
	///  the pointer is dereferenced.
	///
	///  PhysPtr constraints the pointer to a given type.
	template<class T>
	class PhysPtr {
	public:
		PhysPtr() noexcept
		    : m_ptr(nullptr) {}

		explicit PhysPtr(T* ptr) noexcept
		    : m_ptr(ptr) {}

		[[nodiscard]] constexpr T* get() { return m_ptr; }

		[[nodiscard]] constexpr T const* get() const { return m_ptr; }

		T* get_mapped() { return _to_identity_space(m_ptr); }

		[[nodiscard]] T const* get_mapped() const { return _to_identity_space(m_ptr); }

		T& operator*() { return *_to_identity_space(m_ptr); }

		T const& operator*() const { return *_to_identity_space(m_ptr); }

		T& operator[](size_t index) { return *(_to_identity_space(m_ptr) + index); }

		T const& operator[](size_t index) const { return *(_to_identity_space(m_ptr) + index); }

		T* operator->() { return _to_identity_space(m_ptr); }

		T const* operator->() const { return _to_identity_space(m_ptr); }

		constexpr PhysPtr& operator++() {
			m_ptr++;
			return *this;
		}

		constexpr PhysPtr operator++(int) {
			auto temp = *this;
			operator++();
			return temp;
		}

		constexpr PhysPtr operator+(size_t offset) const {
			auto temp = *this;
			temp.m_ptr += offset;
			return temp;
		}

		constexpr PhysPtr operator-(size_t offset) const {
			auto temp = *this;
			temp.m_ptr -= offset;
			return temp;
		}

		constexpr size_t operator-(PhysPtr other_ptr) const { return m_ptr - other_ptr.m_ptr; }

		constexpr PhysPtr& operator=(T* ptr) {
			m_ptr = ptr;
			return *this;
		}

		//  Semantics of operator bool on a physical pointer is not well defined,
		//  as a zero physical pointer is perfectly valid. This may be removed in
		//  the future.
		constexpr explicit operator bool() const { return m_ptr != nullptr; }
	private:
		T* m_ptr;

		static inline T* _to_identity_space(T* ptr) { return reinterpret_cast<T*>(idmap(ptr)); }
	};

	///  Physical address container class
	///  This can be used to distinguish between virtual and physical pointers
	///  and to avoid bugs related to the confusion of the two.
	///
	///	 A PhysAddr contains no type information, it is a bare void* pointer.
	class PhysAddr {
	public:
		PhysAddr() noexcept
		    : m_ptr(nullptr) {}

		explicit PhysAddr(void* addr) noexcept
		    : m_ptr(addr) {}

		template<class T>
		PhysPtr<T> as() {
			return PhysPtr<T> { reinterpret_cast<T*>(m_ptr) };
		}

		constexpr void* get() { return m_ptr; }

		void* get_mapped() { return as<uint8_t>().get_mapped(); }

		PhysAddr operator+(size_t offset) const {
			auto temp = *this;
			temp.operator+=(offset);
			return temp;
		}

		PhysAddr operator-(size_t offset) const {
			auto temp = *this;
			temp.operator-=(offset);
			return temp;
		}

		size_t operator-(PhysAddr v) const { return (uintptr_t)m_ptr - (uintptr_t)v.m_ptr; }

		PhysAddr& operator+=(size_t offset) {
			m_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ptr) + offset);
			return *this;
		}

		PhysAddr& operator-=(size_t offset) {
			m_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_ptr) - offset);
			return *this;
		}

		constexpr bool operator==(PhysAddr const& addr) const { return m_ptr == addr.m_ptr; }

		constexpr bool operator>(PhysAddr const& v) const { return m_ptr > v.m_ptr; }

		constexpr bool operator<(PhysAddr const& v) const { return m_ptr < v.m_ptr; }

		constexpr bool operator<=(PhysAddr const& v) const { return m_ptr <= v.m_ptr; }

		constexpr bool operator>=(PhysAddr const& v) const { return m_ptr >= v.m_ptr; }
	private:
		void* m_ptr;
	};
}

//  Evil global `using` to avoid changing all users of
//  PhysAddr/PhysPtr for now.
using arch::PhysAddr;
using arch::PhysPtr;
