#include <Arch/x86_64/PortIO.hpp>
#include <Daemons/Kbd/Kbd.hpp>
#include <Debug/klogf.hpp>
#include <Locks/KSemaphore.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Structs/StaticRing.hpp>
#include <SystemTypes.hpp>

static StaticRing<uint8, 1024> s_keyboard_buffer {};
static KSemaphore s_keyboard_semaphore;

void Kbd::kbd_thread() {
	while(true) {
		s_keyboard_semaphore.wait();
		while(!s_keyboard_buffer.empty()) {
			auto byte = s_keyboard_buffer.try_pop();
			klogf("Kbd({}): Byte {x}\n", Thread::current()->tid(), byte.unwrap());
		}
	}
}

DEFINE_MICROTASK(Kbd::kbd_microtask) {
	bool characters_inserted { false };
	while(Ports::in(0x64) & 1u) {
		auto ch = Ports::in(0x60);
		s_keyboard_buffer.try_push(ch);
		characters_inserted = true;
	}
	if(characters_inserted) {
		s_keyboard_semaphore.signal();
	}
}
