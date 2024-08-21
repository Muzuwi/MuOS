#include <Arch/x86_64/PortIO.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Error/Error.hpp>
#include <Core/IRQ/IRQ.hpp>
#include <Core/Log/Logger.hpp>
#include <Daemons/Kbd/Kbd.hpp>
#include <Kernel/ksleep.hpp>
#include <Locks/KSemaphore.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Structs/StaticRing.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("test::kbd", core::log::LogLevel::Debug);

static StaticRing<uint8, 1024> s_keyboard_buffer {};
static KSemaphore s_keyboard_semaphore;

static core::irq::HandlingState keyboard_irq_handler(void*) {
	bool characters_inserted { false };
	while(Ports::in(0x64) & 1u) {
		auto ch = Ports::in(0x60);
		s_keyboard_buffer.try_push(ch);
		characters_inserted = true;
	}
	if(characters_inserted) {
		s_keyboard_semaphore.signal();
	}
	return core::irq::HandlingState::Handled;
}

void Kbd::kbd_thread() {
	const auto maybe_handle = core::irq::request_irq(32 + 1, keyboard_irq_handler, core::irq::IrqLineFlags::SharedIrq);
	if(maybe_handle.has_error()) {
		log.error("Failed to request keyboard IRQ ({})", maybe_handle.error());
	}

	while(true) {
		s_keyboard_semaphore.wait();
		while(!s_keyboard_buffer.empty()) {
			auto byte = s_keyboard_buffer.try_pop();
			log.debug("Kbd({}): Byte {x}", Thread::current()->tid(), byte.unwrap());
		}
	}
}
