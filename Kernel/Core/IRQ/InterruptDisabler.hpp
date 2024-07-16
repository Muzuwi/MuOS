#pragma once
#include <Arch/Interrupt.hpp>

namespace core::irq {

	/*  Temporarily disable interrupts in a given scope.
	 *
	 *  Instantiating this class inhibits reception of interrupts on the
	 *  current node. The interrupts are re-enabled once the object goes
	 *  out of scope. If interrupts were already disabled during construction,
	 *  they are not accidentally re-enabled.
	 */
	class InterruptDisabler {
	public:
		explicit InterruptDisabler() noexcept {
			m_int_state = irq_local_enabled();
			if(m_int_state) {
				irq_local_disable();
			}
		}

		constexpr InterruptDisabler(InterruptDisabler&& lg) noexcept = delete;
		constexpr InterruptDisabler(InterruptDisabler const&) = delete;

		~InterruptDisabler() noexcept {
			if(m_int_state) {
				irq_local_enable();
			}
		}
	private:
		bool m_int_state;
	};

}