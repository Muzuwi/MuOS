#include "DemoVESA.hpp"
#include <Arch/x86_64/V86.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <LibGeneric/Vector.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("test::kbd", core::log::LogLevel::Debug);

//  Hardcoded because this should never be used
#define CONFIG_ARCH_X86_64_V86_VESA_DEMO_PAGE ((void*)0xc000)

void vesademo::demo_thread() {
	const auto tid = Thread::current()->tid();

	log.info("vesademo({}): delaying for 5s", tid);
	Thread::current()->msleep(5000);

	struct VbeInfoBlock {
		char VbeSignature[4];       //  == "VESA"
		uint16 vbe_version;         //  == 0x0300 for VBE 3.0
		uint16 oem_string_farptr[2];//  isa vbeFarPtr
		uint8 capabilities[4];
		uint16 video_mode_farptr[2];//  isa vbeFarPtr
		uint16 total_memory;        //  as # of 64KB blocks
	} __attribute__((packed));

	auto page = PhysAddr { CONFIG_ARCH_X86_64_V86_VESA_DEMO_PAGE };
	const uint16 page_u16 = (uintptr_t)page.get() & 0xFFFFu;

	V86Regs regs { .ax = 0x4F00, .di = (uint16)((uintptr_t)page_u16), .es = 0x0000 };
	V86::run_irq(0x10, regs);

	log.info("vesademo({}): regs: ax={x} bx={x} cx={x} dx={x}", tid, regs.ax, regs.bx, regs.cx, regs.dx);
	log.info("vesademo({}):       es={x} si={x} di={x}", tid, regs.es, regs.si, regs.di);
	if(regs.ax == 0x004f) {
		log.info("vesademo({}): VESA supported!", tid);
		log.info("vesademo({}): VBE signature: {}{}{}{}", tid, page.as<VbeInfoBlock>()->VbeSignature[0],
		         page.as<VbeInfoBlock>()->VbeSignature[1], page.as<VbeInfoBlock>()->VbeSignature[2],
		         page.as<VbeInfoBlock>()->VbeSignature[3]);
		log.info("vesademo({}): VBE version: {x}", tid, page.as<VbeInfoBlock>()->vbe_version);
		log.info("vesademo({}): VBE OEM string ptr: {x}", tid, *(uint32*)(&page.as<VbeInfoBlock>()->oem_string_farptr));
		log.info("vesademo({}): Capabilities: {x}", tid, *(uint32*)(&page.as<VbeInfoBlock>()->capabilities));
		const auto video_far_seg = page.as<VbeInfoBlock>()->video_mode_farptr[1];
		const auto video_far_off = page.as<VbeInfoBlock>()->video_mode_farptr[0];
		PhysAddr video_ptr = PhysAddr { (void*)(video_far_seg * 16 + video_far_off) };
		log.info("vesademo({}): VBE video mode ptr: {x} [phys: {x}]", tid,
		         *(uint32*)(&page.as<VbeInfoBlock>()->video_mode_farptr), (uint32)(uintptr_t)video_ptr.get());
		log.info("vesademo({}): Total memory: {}", tid, *(uint16*)(&page.as<VbeInfoBlock>()->total_memory));

		auto current = video_ptr.as<uint16>();
		gen::Vector<uint16> modes {};
		while(*current != 0xFFFF) {
			modes.push_back(*current);
			++current;
		}

		bool mode_was_set = false;
		for(auto& mode : modes) {
			struct vbe_mode_info_structure {
				uint16 attributes;
				uint8 window_a;
				uint8 window_b;
				uint16 granularity;
				uint16 window_size;
				uint16 segment_a;
				uint16 segment_b;
				uint32 win_func_ptr;
				uint16 pitch;
				uint16 width;
				uint16 height;
				uint8 w_char;
				uint8 y_char;
				uint8 planes;
				uint8 bpp;
				uint8 banks;
				uint8 memory_model;
				uint8 bank_size;
				uint8 image_pages;
				uint8 reserved0;

				uint8 red_mask;
				uint8 red_position;
				uint8 green_mask;
				uint8 green_position;
				uint8 blue_mask;
				uint8 blue_position;
				uint8 reserved_mask;
				uint8 reserved_position;
				uint8 direct_color_attributes;

				uint32 framebuffer;
				uint32 off_screen_mem_off;
				uint16 off_screen_mem_size;
				uint8 reserved1[206];
			} __attribute__((packed));

			regs = { .ax = 0x4F01, .cx = mode, .di = (uint16)((uintptr_t)page_u16), .es = 0x0000 };
			V86::run_irq(0x10, regs);

			if(regs.ax != 0x004f) {
				log.info("vesademo({}): Failed getting mode info for mode {}", tid, mode);
				continue;
			}

			auto mode_data = page.as<vbe_mode_info_structure>();

			if((mode_data->attributes & 0x90) != 0x90) {
				continue;
			}

			const unsigned width = 1024;
			const unsigned height = 768;
			const unsigned bpp = 32;

			log.info("vesademo({}): Mode {x} - {}x{}, {}bpp", tid, mode, mode_data->width, mode_data->height,
			         mode_data->bpp);
			if(!mode_was_set && mode_data->width == width && mode_data->height == height && mode_data->bpp == bpp) {
				regs = { .ax = 0x4F02, .bx = (uint16)(mode | 0x4000), .di = 0x0, .es = 0x0 };
				V86::run_irq(0x10, regs);
				if(regs.ax != 0x004f) {
					log.info("vesademo({}): Failed setting mode!", tid);
					goto finalize;
				}
				mode_was_set = true;

				unsigned int aCoefficient = 0x35E79125, cCoefficient = 0x56596B10, state = 0x5D0B1C11;
				auto rand = [&]() -> unsigned {
					state = (state * aCoefficient) + cCoefficient;
					return state;
				};

				auto framebuffer = PhysAddr { (void*)mode_data->framebuffer };
				uint32 t = 0;
				while(true) {
					for(unsigned y = 0; y < height; y++) {
						auto base = framebuffer + y * mode_data->pitch;
						for(unsigned x = 0; x < width; ++x) {
							auto pixel = base + x * 4;
							uint32 color;

							//  int offset = (x + y * 10 + t) & 0xFF;
							//  color = ((offset & 0xFF) << 16) | (((offset + 85) & 0xFF) << 8) | ((offset + 170) &
							//  0xFF);

							//  Generate a xor pattern with some random noise
							uint8 red = ((x + t) % 256) ^ ((y + t) % 256);
							uint8 green = ((2 * x + t) % 256) ^ ((2 * y + t) % 256);
							uint8 blue = (50 + (rand() % 101) + t) % 256;

							//  Improve contrast a bit
							red = (red | (red >> 1)) & 0xFF;
							green = (green | (green >> 1)) & 0xFF;
							blue = (blue | (blue >> 1)) & 0xFF;
							//  This alone makes a cool pattern! Although it doesn't work well with above.
							//  red = red * (255 + (red >> 2)) >> 8;
							//  green = green * (255 + (green >> 2)) >> 8;
							//  blue = blue * (255 + (blue >> 2)) >> 8;

							color = 0x0 | ((unsigned)red << 16) | ((unsigned)green << 8) | (blue);
							*pixel.as<uint32>() = color;
						}
					}
					++t;
					Thread::current()->msleep(50);
				}
			}
		}
	}

finalize:
	log.info("vesademo({}): demo done, goodbye!", tid);
	this_cpu()->scheduler->block();
	ENSURE_NOT_REACHED();
}
