#pragma once
#include <Core/Error/Error.hpp>
#include <Core/IRQ/IRQ.hpp>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>

namespace core::irq {
	/*  Generic IRQ controller definition
	 *
	 *  This defines methods that all IRQ controllers can implement.
	 */
	struct IrqController {
		const IrqId base;
		const size_t count;

		constexpr IrqController(IrqId base_, size_t count_)
		    : base(base_)
		    , count(count_) {}

		virtual void acknowledge_irq(IrqId) = 0;

		virtual bool irq_is_masked(IrqId) = 0;
		virtual void irq_mask(IrqId) = 0;
		virtual void irq_unmask(IrqId) = 0;
	};
}