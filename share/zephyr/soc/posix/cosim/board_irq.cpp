/*
 * board_irq.cpp
 *
 *  Created on: Dec 17, 2021
 *      Author: mballance
 */

#include "board_irq.h"
#include "ZephyrCosim.h"

/**
 * Configure a static interrupt.
 *
 * posix_isr_declare will populate the interrupt table table with the
 * interrupt's parameters, the vector table and the software ISR table.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 *
 * @param irq_p IRQ line number
 * @param flags [plug it directly (1), or as a SW managed interrupt (0)]
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 */
void posix_isr_declare(
		unsigned int irq_p,
		int flags,
		void isr_p(const void *),
		const void *isr_param_p) {
	ZephyrCosim::inst()->isr_declare(
			irq_p, flags, isr_p, isr_param_p);
}

/*
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * Lower values take priority over higher values.
 *
 * @return N/A
 */
void posix_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags) {
	ZephyrCosim::inst()->irq_prio(irq, prio, flags);
}

