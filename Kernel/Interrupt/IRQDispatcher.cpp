#include <Kernel/SystemTypes.hpp>
#include <LibGeneric/List.hpp>
#include <include/Arch/i386/TrapFrame.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Interrupt/IRQSubscriber.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <LibGeneric/Algorithm.hpp>

static gen::List<IRQSubscriber*> s_irq_handlers[16] {};

extern "C"
void _kernel_irq_dispatch(unsigned irq) {
	if(irq > 7)
		out(0xa0, 0x20);
	out(0x20, 0x20);

	IRQDispatcher::dispatch_irq(irq);
}

/*
 *  Calls all subscribers for the specific IRQ
 */
void IRQDispatcher::dispatch_irq(uint8_t irq) {
	if(irq > 15) return;

	auto& list = s_irq_handlers[irq];
	for(auto& subscriber : list) {
		subscriber->handler()();
	}
}

/*
 *  Registers a new subscriber for an IRQ
 */
void IRQDispatcher::register_subscriber(IRQSubscriber* subscriber, SubscriberPriority priority) {
	IRQDisabler disabler;
	if(!subscriber) return;
	if(subscriber->irq() > 15) return;


	if(priority == SubscriberPriority::Normal) {
		auto& handlers = s_irq_handlers[subscriber->irq()];

		if(!handlers.empty())
			handlers.insert(handlers.end()--, subscriber);
		else
			handlers.push_back(subscriber);
	} else {
		s_irq_handlers[subscriber->irq()].push_back(subscriber);
	}
}

/*
 *  Removes an already existing subscriber for an IRQ
 */
void IRQDispatcher::remove_subscriber(IRQSubscriber* subscriber) {
	IRQDisabler disabler;
	if(!subscriber) return;
	if(subscriber->irq() > 15) return;

	auto& subscriber_list = s_irq_handlers[subscriber->irq()];
	auto it = gen::find(subscriber_list, subscriber);
	if(it != subscriber_list.end())
		subscriber_list.erase(it);
}