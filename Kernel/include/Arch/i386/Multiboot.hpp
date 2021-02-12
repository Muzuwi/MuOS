#pragma once

/*
	This header contains things useful for parsing multiboot
*/
enum multiboot_flag_t {
	MULTIBOOT_MEMINFO = (1 << 0),
	MULTIBOOT_BOOTDEVICE = (1 << 1),
	MULTIBOOT_CMDLINE = (1 << 2),
	MULTIBOOT_MODULES = (1 << 3),
	MULTIBOOT_KSYMBOLS = (1 << 4),
	MULTIBOOT_KSECTIONS = (1 << 4),
	MULTIBOOT_MEMMAP  = (1 << 6),
	MULTIBOOT_DRIVES  = (1 << 7),
	MULTIBOOT_CONFT   = (1 << 8),
	MULTIBOOT_BOOTNAME = (1 << 9),
	MULTIBOOT_APM = (1 << 10),
	MULTIBOOT_VBE = (1 << 11),
	MULTIBOOT_FBINFO = (1 << 12),
};

enum mmap_memory_type_t {
};