#pragma once
#include <LibGeneric/Vector.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/String.hpp>

enum ABI {
	SysV    = 0,
	Linux   = 4
};

enum Arch {
	Invalid = 0,
	x32 = 1,
	x64 = 2
};

enum Encoding {
	LSB = 1,
	MSB = 2
};

enum ObjectType {
	Relocatable = 1,
	Executable  = 2,
	Shared      = 3,
	Core        = 4
};

enum SegType {
	Null = 0,
	Load = 1,
	Dynamic  = 2,
	Interpreter = 3,
	Note = 4,
	ShLib = 5,
	ProgHeader = 6,
};

enum PermFlags {
	Read = 0x4,
	Write = 0x2,
	Execute = 0x1
};

struct ELFHeader32 {
	unsigned char e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} __attribute__((packed));
struct ELFHeader64 {
	unsigned char e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} __attribute__((packed));

struct SectionHeader32 {
	uint32_t   sh_name;
	uint32_t   sh_type;
	uint32_t   sh_flags;
	uint32_t   sh_addr;
	uint32_t   sh_offset;
	uint32_t   sh_size;
	uint32_t   sh_link;
	uint32_t   sh_info;
	uint32_t   sh_addralign;
	uint32_t   sh_entsize;
} __attribute__((packed));
struct SectionHeader64 {
	uint32_t   sh_name;
	uint32_t   sh_type;
	uint64_t   sh_flags;
	uint64_t   sh_addr;
	uint64_t   sh_offset;
	uint64_t   sh_size;
	uint32_t   sh_link;
	uint32_t   sh_info;
	uint64_t   sh_addralign;
	uint64_t   sh_entsize;
} __attribute__((packed));

struct Symbol32 {
	uint32_t      st_name;
	uint32_t      st_value;
	uint32_t      st_size;
	unsigned char st_info;
	unsigned char st_other;
	uint16_t      st_shndx;
} __attribute__((packed));
struct Symbol64 {
	uint32_t      st_name;
	unsigned char st_info;
	unsigned char st_other;
	uint16_t      st_shndx;
	uint64_t      st_value;
	uint64_t      st_size;
} __attribute__((packed));

struct ProgHeader32 {
	uint32_t   p_type;
	uint32_t   p_offset;
	uint32_t   p_vaddr;
	uint32_t   p_paddr;
	uint32_t   p_filesz;
	uint32_t   p_memsz;
	uint32_t   p_flags;
	uint32_t   p_align;
} __attribute__((packed));
struct ProgHeader64 {
	uint32_t   p_type;
	uint32_t   p_flags;
	uint64_t   p_offset;
	uint64_t   p_vaddr;
	uint64_t   p_paddr;
	uint64_t   p_filesz;
	uint64_t   p_memsz;
	uint64_t   p_align;
} __attribute__((packed));

struct Architecture32 {
	typedef ELFHeader32 ElfHeaderType;
	typedef SectionHeader32 SectionHeaderType;
	typedef Symbol32 SymbolType;
	typedef ProgHeader32 ProgHeaderType;
	static const Arch ArchByte = Arch::x32;
};

struct Architecture64 {
	typedef ELFHeader64 ElfHeaderType;
	typedef SectionHeader64 SectionHeaderType;
	typedef Symbol64 SymbolType;
	typedef ProgHeader64 ProgHeaderType;
	static const Arch ArchByte = Arch::x64;
};

template<typename Architecture>
class ELFParser {
public:
	typedef typename Architecture::ElfHeaderType ElfHeaderType;
	typedef typename Architecture::SectionHeaderType SectHeaderType;
	typedef typename Architecture::SymbolType SymbolType;
	typedef typename Architecture::ProgHeaderType ProgHeaderType;
private:
	void* m_elf_phys;
	size_t m_elf_size;

	uint64_t m_entry;
	ObjectType m_type;
	gen::vector<SectHeaderType> m_sections;
	gen::vector<ProgHeaderType> m_prog_headers;

	void parse_sect_headers();
	void parse_prog_headers();
	void parse_header();

	ELFParser(void* elf, size_t);

public:
	static Arch arch_from_image(void* elf, size_t);
	static ELFParser* from_image(void* elf, size_t);

	gen::String string_lookup(size_t index) const;
	gen::vector<SectHeaderType> sections() const;
	gen::vector<ProgHeaderType> program_headers() const;
	void* entrypoint() const { return reinterpret_cast<void*>(m_entry); }
	ObjectType object_type() const { return m_type; }
};

typedef ELFParser<Architecture32> ELFParser32;
typedef ELFParser<Architecture64> ELFParser64;