#pragma once

/* IRQ state manipulation macros */

#ifdef ARCH_IS_x86_64
#	include <Arch/x86_64/Interrupt.hpp>
#endif

#ifndef irq_local_disable
#	warning irq_local_disable not provided by architecture and will be a no-op!
/** Disable the reception of IRQs on the current node.
 */
#	define irq_local_disable()
#endif

#ifndef irq_local_enable
#	warning irq_local_enable not provided by architecture and will be a no-op!
/** Enable the reception of IRQs on the current node.
 */
#	define irq_local_enable()
#endif

#ifndef irq_local_enabled
#	warning irq_local_enabled not provided by architecture and will be a no-op!
/** Query IRQ reception state for the current node.
 *  Should evaluate to true if the current node can receive IRQs.
 */
#	define irq_local_enabled() (false)
#endif