#ifdef __is_kernel_build
#	include <Core/Assert/Assert.hpp>
#endif
//
//  #include <LibGeneric/ELFParser.hpp>
//
//  template<class T>
//  bool ensure_within(void* image_base, size_t image_size, void* address) {
//	return (address >= image_base) && ((uintptr_t)address + sizeof(T) <= (uintptr_t)image_base + image_size);
//}
//
//  template<size_t struct_size>
//  bool ensure_within(void* image_base, size_t image_size, void* address) {
//	return (address >= image_base) && ((uintptr_t)address + struct_size <= (uintptr_t)image_base + image_size);
//}
//
//  template<typename Architecture>
//  gen::String ELFParser<Architecture>::string_lookup(size_t index) const {
//	auto* elfHeader = reinterpret_cast<ElfHeaderType*>(m_elf_phys);
//	auto offset = elfHeader->e_shoff;
//	auto entrySize = elfHeader->e_shentsize;
//	auto symEntry  = elfHeader->e_shstrndx;
//
//	if(symEntry == 0xffff) {
//		auto* firstSectionHeader = reinterpret_cast<SectHeaderType*>((uint64_t)m_elf_phys + offset);
//		if(!ensure_within<SectHeaderType>(m_elf_phys, m_elf_size, firstSectionHeader))
//			return {};
//
//		symEntry = firstSectionHeader->sh_link;
//	}
//
//	auto* stringTableHeader = reinterpret_cast<SectHeaderType*>((uint64_t)m_elf_phys + offset + symEntry*entrySize);
//	if(!ensure_within<SectHeaderType>(m_elf_phys, m_elf_size, stringTableHeader))
//		return {};
//
//	auto* c_str = reinterpret_cast<char*>((uint64_t)m_elf_phys + stringTableHeader->sh_offset + index);
//	if(!ensure_within<char>(m_elf_phys, m_elf_size, c_str))
//		return {};
//	return {c_str};
//}
//
//
//  template<typename Architecture>
//  void ELFParser<Architecture>::parse_prog_headers() {
//	auto* elfHeader = reinterpret_cast<ElfHeaderType*>(m_elf_phys);
//	auto offset    = elfHeader->e_phoff;
//	auto entrySize = elfHeader->e_phentsize;
//	auto entries   = elfHeader->e_phnum;
//
////	m_prog_headers.resize(entries);
//
//	for(uint16_t i = 0; i < entries; ++i) {
//		auto* header = reinterpret_cast<ProgHeaderType*>((uint64_t)m_elf_phys + offset + i * entrySize);
//		if(!ensure_within<ProgHeaderType>(m_elf_phys, m_elf_size, header))
//			continue;
//
////		m_prog_headers[i] = *header;
//	}
//}
//
//  template<typename Architecture>
//  void ELFParser<Architecture>::parse_sect_headers() {
//	auto* elfHeader = reinterpret_cast<ElfHeaderType*>(m_elf_phys);
//	auto offset = elfHeader->e_shoff;
//	auto entrySize = elfHeader->e_shentsize;
//	auto entries   = elfHeader->e_shnum;
//	auto symEntry  = elfHeader->e_shstrndx;
//
////	m_sections.resize(entries);
//
//	for(uint16_t i = 0; i < entries; ++i) {
//		auto* header = reinterpret_cast<SectHeaderType*>((uint64_t)m_elf_phys + offset + i * entrySize);
//		if(!ensure_within<SectHeaderType>(m_elf_phys, m_elf_size, header))
//			return;
//
//		if(symEntry == 0xffff && i == 0) {
//			symEntry = header->sh_link;
//		}
//
////		m_sections[i] = *header;
//
//	}
//}
//
//  template<typename Architecture>
//  void ELFParser<Architecture>::parse_header() {
//	auto* header = reinterpret_cast<ElfHeaderType*>(m_elf_phys);
//	if(!ensure_within<ElfHeaderType>(m_elf_phys, m_elf_size, header))
//		return;
//
//	m_entry = header->e_entry;
//
//	if(header->e_type <= 4)
//		m_type = static_cast<ObjectType>(header->e_type);
//	else
//		return;
//
//	this->parse_sect_headers();
//	this->parse_prog_headers();
//}
//
//  template<typename Architecture>
//  ELFParser<Architecture>::ELFParser(void* base, size_t size) {
//	m_elf_phys = base;
//	m_elf_size = size;
//
//	this->parse_header();
//}
//
////template<typename Architecture>
////gen::vector<typename ELFParser<Architecture>::SectHeaderType> ELFParser<Architecture>::sections() const {
////	//  FIXME:  Fix Vector in LibGeneric to take const reference in copy constructor instead of non-const
////	return gen::vector<SectHeaderType>(const_cast<gen::vector<SectHeaderType>&>(m_sections));
////}
////
////template<typename Architecture>
////gen::vector<typename ELFParser<Architecture>::ProgHeaderType> ELFParser<Architecture>::program_headers() const {
////	return gen::vector<ProgHeaderType>(const_cast<gen::vector<ProgHeaderType>&>(m_prog_headers));
////}
//
//  template<typename Architecture>
//  ELFParser<Architecture>* ELFParser<Architecture>::from_image(void* elf, size_t size) {
//	auto* identity = reinterpret_cast<unsigned char*>(elf);
//	if(!ensure_within<16>(elf, size, identity))
//		return nullptr;
//
//	//  Wrong magic
//	if(!(identity[0] == 0x7f && identity[1] == 'E' && identity[2] == 'L' && identity[3] == 'F'))
//		return nullptr;
//
//	//  Wrong arch
//	if(!(identity[4] == Arch::x32 || identity[4] == Arch::x64) || identity[4] != Architecture::ArchByte)
//		return nullptr;
//
//	//  Invalid encoding
//	if(!(identity[5] == Encoding::LSB || identity[5] == Encoding::MSB))
//		return nullptr;
//
//	//  Invalid version
//	if(identity[6] == 0)
//		return nullptr;
//
//	//  Unsupported ABI
//	if(!(identity[7] == ABI::SysV || identity[7] == ABI::Linux))
//		return nullptr;
//
//	return new ELFParser<Architecture>(elf, size);
//}
//
//  template<typename Architecture>
//  Arch ELFParser<Architecture>::arch_from_image(void* elf, size_t size) {
//	auto* identity = reinterpret_cast<unsigned char*>(elf);
//	if(!ensure_within<16>(elf, size, identity))
//		return Arch::Invalid;
//
//	if(identity[4] == Arch::x32 || identity[4] == Arch::x64)
//		return static_cast<Arch>(identity[4]);
//	else
//		return Arch::Invalid;
//}
//
//  template class ELFParser<Architecture32>;
//  template class ELFParser<Architecture64>;