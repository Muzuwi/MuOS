#include <Arch/Interrupt.hpp>
#include <Core/Error/Error.hpp>
#include <Core/IRQ/Controller.hpp>
#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/IRQ/IRQ.hpp>
#include <LibGeneric/Algorithm.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Move.hpp>
#include <LibGeneric/Optional.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <LibGeneric/Utility.hpp>
#include <SystemTypes.hpp>

struct IrqHandler {
	core::irq::IrqId id;
	core::irq::MicrotaskHandler functor;
	core::irq::IrqLineFlags flags;
	core::irq::HandlerToken token;
};

static gen::StaticVector<core::irq::IrqController*, 32> s_controllers;
static gen::StaticVector<IrqHandler, 64> s_interrupt_handlers;
static gen::Spinlock s_lock;

/*	Hash a given object
 *
 *	Interpret the given object as bytes and hash them using FNV-1.
 */
template<typename T>
static uint64 hashof(T const& value) {
	constexpr uint64 FNV_OFFSET_BASIS = 0xcbf29ce484222325;
	constexpr uint64 FNV_PRIME = 0x100000001b3;

	struct TypeIntrospection {
		alignas(T) uint8 bytes[sizeof(T)];
	} __attribute__((packed));
	const auto v = gen::bitcast<TypeIntrospection>(value);

	uint64 hash = FNV_OFFSET_BASIS;
	for(size_t i = 0; i < sizeof(v.bytes); ++i) {
		hash ^= static_cast<uint64>(v.bytes[i]);
		hash *= FNV_PRIME;
	}
	return hash;
}

/*	Find a handler for the given IRQ line, and optionally, with a given token.
 */
static IrqHandler* find_handler_for_irq(core::irq::IrqId id, core::irq::HandlerToken token = nullptr) {
	for(auto& handler : s_interrupt_handlers) {
		if(token && handler.token == token) {
			break;
		}
		if(handler.id == id) {
			return &handler;
		}
	}
	return nullptr;
}

/*	Find the controller associated with the given IRQ line.
 */
static core::irq::IrqController* find_controller_for_irq(core::irq::IrqId id) {
	auto it = gen::find_if(s_controllers, [id](core::irq::IrqController* controller) -> bool {
		return id >= controller->base && id < controller->base + controller->count;
	});
	return it == s_controllers.end() ? nullptr : *it;
}

static IrqHandler* add_handler(IrqHandler handler) {
	s_interrupt_handlers.push_back(gen::move(handler));
	return &s_interrupt_handlers.back();
}

static core::Error add_controller(core::irq::IrqController* controller) {
	if(gen::find(s_controllers, controller) != s_controllers.end()) {
		return core::Error::EntityAlreadyExists;
	}

	s_controllers.push_back(controller);
	return core::Error::Ok;
}

static core::Error remove_controller(core::irq::IrqController* controller) {
	gen::Optional<size_t> idx = { gen::nullopt };
	//  Find the entry
	for(size_t i = 0; i < s_controllers.size(); ++i) {
		if(s_controllers[i] == controller) {
			idx = gen::Optional<size_t> { i };
		}
	}
	if(!idx.has_value()) {
		return core::Error::EntityMissing;
	}
	//  Move remaining entries downwards
	for(size_t i = idx.unwrap(); i < s_controllers.size() - 1; ++i) {
		s_controllers[i] = s_controllers[i + 1];
	}
	(void)s_controllers.pop_back();
	return core::Error::Ok;
}

static core::Error remove_handler_for_irq(core::irq::IrqId id, core::irq::HandlerToken token) {
	gen::Optional<size_t> idx { gen::nullopt };
	//  Find the entry
	for(size_t i = 0; i < s_interrupt_handlers.size(); ++i) {
		if(s_interrupt_handlers[i].id == id && s_interrupt_handlers[i].token == token) {
			idx = gen::Optional<size_t> { i };
		}
	}
	if(!idx.has_value()) {
		return core::Error::EntityMissing;
	}
	//  Move remaining entries downwards
	for(size_t i = idx.unwrap(); i < s_interrupt_handlers.size() - 1; ++i) {
		s_interrupt_handlers[i] = s_interrupt_handlers[i + 1];
	}
	(void)s_interrupt_handlers.pop_back();
	return core::Error::Ok;
}

/*	Try releasing the given IRQ line.
 *
 *	Checks if an IRQ line is used by any other existing handlers, and if it isn't
 *	then the given IRQ is remasked.
 */
static core::Error try_release_irq(core::irq::IrqId id) {
	//  Find a controller that is responsible for this interrupt line
	auto* controller = find_controller_for_irq(id);
	if(!controller) {
		return core::Error::EntityMissing;
	}

	auto* other = find_handler_for_irq(id);
	if(!other) {
		controller->irq_mask(id);
		return core::Error::Ok;
	}
	return core::Error::Ok;
}

/*	Execute all handlers for a given IRQ line.
 *
 * 	Given `data` cookie is passed to each of the handlers.
 */
static void run_handlers_for_irq(core::irq::IrqId id, void* data) {
	for(auto& handler : s_interrupt_handlers) {
		if(handler.id != id) {
			continue;
		}
		const auto value = handler.functor(gen::move(data));
		if(value == core::irq::HandlingState::Handled) {
			break;
		}
	}
}

/*	Create a HandlerToken for the given handler struct.
 *
 *	The functor object is hashed, as that should give relatively good
 *	uniqueness guarantees.
 */
static inline core::irq::HandlerToken create_token_for_handler(IrqHandler& handler) {
	//  Smuggle the 64-bit value in a void*
	return reinterpret_cast<core::irq::HandlerToken>(hashof(handler.functor));
}

/*	Try creating a new handler for the given IRQ line.
 *
 *	Constructs an IrqHandler and ensures that shared IRQ line-related
 *	invariants are kept.
 */
static IrqHandler* try_create_handler(core::irq::IrqId id, core::irq::MicrotaskHandler functor,
                                      core::irq::IrqLineFlags flags) {
	auto new_handler = IrqHandler { .id = id, .functor = gen::move(functor), .flags = flags, .token = nullptr };
	new_handler.token = create_token_for_handler(new_handler);

	//  Try looking for an existing IRQ handler
	//  This is required to handle IRQ sharing
	auto* existing_handler = find_handler_for_irq(id);
	if(existing_handler) {
		const bool existing_is_shared = static_cast<uintptr_t>(existing_handler->flags) &
		                                static_cast<uintptr_t>(core::irq::IrqLineFlags::SharedIrq);
		const bool new_is_shared =
		        static_cast<uintptr_t>(flags) & static_cast<uintptr_t>(core::irq::IrqLineFlags::SharedIrq);

		//  We can only add a new handler if this is a shared IRQ line:
		//	1) If an existing line is shared, but the request is for an exclusive line,
		//	   the caller is too late and the line cannot be registered as exclusive already.
		//  2) If an existing line is exclusive, then the new request's type is irrelevant
		//	   as the line is already taken
		if(!(existing_is_shared && new_is_shared)) {
			return nullptr;
		}
	}

	return add_handler(gen::move(new_handler));
}

core::Error core::irq::register_controller(IrqController* controller) {
	core::irq::InterruptDisabler disabler {};
	gen::LockGuard lg { s_lock };

	return add_controller(controller);
}

core::Error core::irq::unregister_controller(IrqController* controller) {
	core::irq::InterruptDisabler disabler {};
	gen::LockGuard lg { s_lock };

	return remove_controller(controller);
}

core::Result<core::irq::HandlerToken> core::irq::request_irq(IrqId id, MicrotaskHandler functor, IrqLineFlags flags) {
	core::irq::InterruptDisabler disabler {};
	gen::LockGuard lg { s_lock };

	//  Find a controller that is responsible for this interrupt line
	auto* controller = find_controller_for_irq(id);
	if(!controller) {
		return core::Result<core::irq::HandlerToken> { Error::EntityMissing };
	}
	const bool other_handlers_exist = find_handler_for_irq(id) != nullptr;

	auto* new_handler = try_create_handler(id, gen::move(functor), flags);
	if(!new_handler) {
		return core::Result<core::irq::HandlerToken> { Error::EntityAlreadyExists };
	}

	//  When requesting a shared IRQ for the first time, unmask the line.
	//  Subsequent requests don't need to unmask it again.
	if(!other_handlers_exist) {
		controller->irq_unmask(id);
	}
	return core::Result<core::irq::HandlerToken> { new_handler->token };
}

core::Error core::irq::release_irq(IrqId id, core::irq::HandlerToken token) {
	core::irq::InterruptDisabler disabler {};
	gen::LockGuard lg { s_lock };

	if(!token) {
		return Error::InvalidArgument;
	}

	const auto err = remove_handler_for_irq(id, token);
	if(err != Error::Ok) {
		return Error::EntityMissing;
	}

	(void)try_release_irq(id);

	return Error::Ok;
}

core::Error core::irq::configure_irq(IrqId id, IrqConfiguration config) {
	core::irq::InterruptDisabler disabler {};
	gen::LockGuard lg { s_lock };

	auto* controller = find_controller_for_irq(id);
	if(!controller) {
		return Error::EntityMissing;
	}

	if(config.mask == MaskState::Masked) {
		controller->irq_mask(id);
	} else if(config.mask == MaskState::Unmasked) {
		controller->irq_unmask(id);
	}
	//  TODO: Add trigger type configuration to controllers

	return Error::Ok;
}

void core::irq::dispatch(IrqId id, void* data) {
	core::irq::InterruptDisabler disabler {};
	gen::LockGuard lg { s_lock };

	run_handlers_for_irq(id, data);

	auto* controller = find_controller_for_irq(id);
	if(controller) {
		controller->acknowledge_irq(id);
	}
}
