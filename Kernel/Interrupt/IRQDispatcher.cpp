#include <Kernel/SystemTypes.hpp>
#include <LibGeneric/List.hpp>
#include <include/Arch/i386/TrapFrame.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Interrupt/IRQSubscriber.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <Kernel/Debug/kdebugf.hpp>

static gen::List<IRQSubscriber*> s_irq_handlers[16];

extern "C"
void _kernel_irq_dispatch(unsigned irq, TrapFrame trapFrame) {
	if(irq > 7)
		out(0xa0, 0x20);
	out(0x20, 0x20);

	IRQDispatcher::dispatch_irq(irq, trapFrame);
}

/*
 *  Calls all subscribers for the specific IRQ
 */
void IRQDispatcher::dispatch_irq(uint8_t irq, TrapFrame& frame) {
	if(irq > 15) return;

	auto& list = s_irq_handlers[irq];
	for(auto& subscriber : list) {
		subscriber->handler()(frame);
	}
}

/*
 *  Registers a new subscriber for an IRQ
 */
void IRQDispatcher::register_subscriber(IRQSubscriber* subscriber) {
	IRQDisabler disabler;
	if(!subscriber) return;
	if(subscriber->irq() > 15) return;

	s_irq_handlers[subscriber->irq()].push_back(subscriber);
}

/*
 *  Removes an already existing subscriber for an IRQ
 */
void IRQDispatcher::remove_subscriber(IRQSubscriber* subscriber) {
	IRQDisabler disabler;
	if(!subscriber) return;
	if(subscriber->irq() > 15) return;

	//  FIXME: Move this to LibGeneric
	auto find = [](gen::BidirectionalIterator<gen::List<IRQSubscriber*>> begin, gen::BidirectionalIterator<gen::List<IRQSubscriber*>> end, IRQSubscriber* val){
		auto it = begin;
		while(it != end) {
			if((*it) == val) return it;
			++it;
		}
		return it;
	};

	auto& subscriber_list = s_irq_handlers[subscriber->irq()];
	auto it = find(subscriber_list.begin(), subscriber_list.end(), subscriber);
	if(it != subscriber_list.end())
		subscriber_list.erase(it);
}