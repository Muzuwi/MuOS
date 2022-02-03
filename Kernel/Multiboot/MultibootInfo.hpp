#pragma once
#include <stdint.h>
#include <Memory/Ptr.hpp>

class MultibootMMap {
public:
	enum class RegionType : uint32_t {
		USABLE = 1,
		ACPI   = 3,
		HIBERN = 4,
		BAD    = 5
	};
private:
	uint32_t m_size;
	uint64_t m_start;
	uint64_t m_range;
	RegionType m_type;
public:
	uint32_t size() const {
		return m_size;
	}

	uint64_t start() const {
		return m_start;
	}

	uint64_t range() const {
		return m_range;
	}

	RegionType type() const {
		return m_type;
	}

	MultibootMMap const* next_entry() const {
		return reinterpret_cast<MultibootMMap const*>(reinterpret_cast<uint8_t const*>(this)+m_size+4);
	}

	MultibootMMap* next_entry() {
		return reinterpret_cast<MultibootMMap*>(reinterpret_cast<uint8_t*>(this)+m_size+4);
	}
} __attribute__((packed));

class MultibootInfo {
	/* Multiboot info version number */
	uint32_t flags;

	/* Available memory from BIOS */
	uint32_t mem_lower;
	uint32_t mem_upper;

	/* "root" partition */
	uint32_t boot_device;

	/* Kernel command line */
	uint32_t cmdline;

	/* Boot-Module list */
	uint32_t mods_count;
	uint32_t mods_addr;

	uint32_t elf_section_num;
	uint32_t elf_section_size;
	uint32_t elf_section_addr;
	uint32_t elf_section_shndx;

	/* Memory Mapping buffer */
	uint32_t mmap_length;
	uint32_t mmap_addr;

	/* Drive Info buffer */
	uint32_t drives_length;
	uint32_t drives_addr;

	/* ROM configuration table */
	uint32_t config_table;

	/* Boot Loader Name */
	uint32_t boot_loader_name;

	/* APM table */
	uint32_t apm_table;

	/* Video */
	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint16_t vbe_mode;
	uint16_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;

	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;

	uint8_t framebuffer_type;
	union
	{
		struct
		{
			uint32_t framebuffer_palette_addr;
			uint16_t framebuffer_palette_num_colors;
		};
		struct
		{
			uint8_t framebuffer_red_field_position;
			uint8_t framebuffer_red_mask_size;
			uint8_t framebuffer_green_field_position;
			uint8_t framebuffer_green_mask_size;
			uint8_t framebuffer_blue_field_position;
			uint8_t framebuffer_blue_mask_size;
		};
	};
public:
	[[nodiscard]] PhysPtr<MultibootMMap> mmap() const {
		return PhysPtr<MultibootMMap>(reinterpret_cast<MultibootMMap*>(mmap_addr));
	}

	[[nodiscard]] PhysPtr<MultibootMMap> mmap_end() const {
		return PhysPtr<MultibootMMap>(reinterpret_cast<MultibootMMap*>(mmap_addr+mmap_length));
	}
} __attribute__((packed));
