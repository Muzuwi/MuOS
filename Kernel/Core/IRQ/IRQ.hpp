#pragma once
#include <Core/Error/Error.hpp>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>

/*	core::irq - generic handling of platform interrupts
 *
 * 	This subsystem is responsible for dispatching platform-level
 * 	interrupts to handlers and manages IRQ line allocation.
 */
namespace core::irq {
	struct IrqController;

	enum class IrqLineFlags {
		/* Specify that this IRQ can be shared */
		SharedIrq = 1 << 0,
	};

	enum class HandlingState {
		/* Should be returned by shared handlers only, indicates that
		   this handler was not the one that handles the shared IRQ */
		NotDone,
		/* Must be returned by all handlers on exit */
		Handled,
	};

	enum class TriggerType {
		LevelHigh = 0,
		LevelLow = 1,
		EdgeRising = 2,
		EdgeFalling = 3
	};

	enum class MaskState {
		Unmasked = 0,
		Masked = 1
	};

	struct IrqConfiguration {
		TriggerType type;
		MaskState mask;

		constexpr IrqConfiguration(TriggerType type, MaskState mask)
		    : type(type)
		    , mask(mask) {}
	};

	using MicrotaskHandler = KFunction<HandlingState(void*)>;
	using IrqId = uint64;
	using HandlerToken = void*;

	/*	Register an IRQ controller
	 *
	 * 	Registers a given IRQ controller, which abstracts away the handling
	 * 	of interrupt configuration operations on a given range of IRQ lines.
	 */
	core::Error register_controller(IrqController*);

	/*	Remove an IRQ controller
	 *
	 * 	Removes a previously registered controller. This operation may fail if
	 *	there still exist any handlers for any interrupt lines managed by this
	 * 	controller.
	 */
	core::Error unregister_controller(IrqController*);

	/*	Request an IRQ line handler
	 *
	 * 	This allows registering an IRQ handler to be called when an interrupt
	 * 	on the line given by IrqId occurs. By default, the request is assumed
	 * 	to be for an exclusive IRQ line, and no other handlers are allowed to
	 *  be registered for this line. If you want to share the IRQ line with
	 *	other kernel handlers, pass the IrqLineFlags::SharedIrq flag.
	 *
	 * 	On success, returns a HandlerToken that can be used to free the IRQ later.
	 */
	core::Result<HandlerToken> request_irq(IrqId, MicrotaskHandler, IrqLineFlags);

	/*	Release a previously allocated IRQ line handler
	 *
	 *	This removes the previously allocated handler for a given IRQ line. If
	 *	the line is no longer used by any other handlers, the IRQ line is disabled.
	 *	You must pass the HandlerToken that was returned from the matching request_irq
	 * 	used to allocate the line.
	 */
	core::Error release_irq(IrqId, HandlerToken);

	/*	Configure an IRQ line.
	 *
	 * 	Allows changing the trigger type and masking state of a given IRQ.
	 * 	NOTE: Trigger type is currently unimplemented!
	 */
	core::Error configure_irq(IrqId, IrqConfiguration);

	/*	Dispatch an interrupt on the given line
	 *
	 *	This is relevant only for the platform implementation, you will never
	 *	need to use this.
	 *
	 *	This function must be called by the underlying platform/IRQ controller
	 * 	driver implementation to inform the subsystem that an interrupt occured
	 * 	and to call registered handlers.
	 */
	void dispatch(IrqId, void*);
}