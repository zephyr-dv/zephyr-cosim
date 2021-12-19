/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SW side of the IRQ handling
 */

#include <stdint.h>
#include "irq_handler.h"
#include "irq_offload.h"
#include "kernel_structs.h"
#include "kernel_internal.h"
#include "kswap.h"
#include "irq_ctrl.h"
#include "posix_core.h"
#include "board_soc.h"
#include "sw_isr_table.h"
#include "soc.h"
#include <tracing/tracing.h>

typedef void (*normal_irq_f_ptr)(const void *);
typedef int (*direct_irq_f_ptr)(void);

typedef struct _isr_list isr_table_entry_t;
static isr_table_entry_t irq_vector_table[N_IRQS] = { { 0 } };

static int currently_running_irq = -1;

static inline void vector_to_irq(int irq_nbr, int *may_swap)
{
	sys_trace_isr_enter();

	if (irq_vector_table[irq_nbr].func == NULL) { /* LCOV_EXCL_BR_LINE */
		/* LCOV_EXCL_START */
		posix_print_error_and_exit("Received irq %i without a "
					"registered handler\n",
					irq_nbr);
		/* LCOV_EXCL_STOP */
	} else {
		if (irq_vector_table[irq_nbr].flags & ISR_FLAG_DIRECT) {
			*may_swap |= ((direct_irq_f_ptr)
					irq_vector_table[irq_nbr].func)();
		} else {
#ifdef CONFIG_PM
			posix_irq_check_idle_exit();
#endif
			((normal_irq_f_ptr)irq_vector_table[irq_nbr].func)
					(irq_vector_table[irq_nbr].param);
			*may_swap = 1;
		}
	}

	sys_trace_isr_exit();
}

/**
 * When an interrupt is raised, this function is called to handle it and, if
 * needed, swap to a re-enabled thread
 *
 * Note that even that this function is executing in a Zephyr thread,  it is
 * effectively the model of the interrupt controller passing context to the IRQ
 * handler and therefore its priority handling
 */
void posix_irq_handler(void)
{
	uint64_t irq_lock;
	int irq_nbr;
	static int may_swap;

	irq_lock = hw_irq_ctrl_get_current_lock();

	if (irq_lock) {
		/* "spurious" wakes can happen with interrupts locked */
		return;
	}

	if (_kernel.cpus[0].nested == 0) {
		may_swap = 0;
	}

	_kernel.cpus[0].nested++;

	while ((irq_nbr = hw_irq_ctrl_get_highest_prio_irq()) != -1) {
		int last_current_running_prio = hw_irq_ctrl_get_cur_prio();
		int last_running_irq = currently_running_irq;

		hw_irq_ctrl_set_cur_prio(hw_irq_ctrl_get_prio(irq_nbr));
		hw_irq_ctrl_clear_irq(irq_nbr);

		currently_running_irq = irq_nbr;
		vector_to_irq(irq_nbr, &may_swap);
		currently_running_irq = last_running_irq;

		hw_irq_ctrl_set_cur_prio(last_current_running_prio);
	}

	_kernel.cpus[0].nested--;

	/* Call swap if all the following is true:
	 * 1) may_swap was enabled
	 * 2) We are not nesting irq_handler calls (interrupts)
	 * 3) Next thread to run in the ready queue is not this thread
	 */
	if (may_swap
		&& (hw_irq_ctrl_get_cur_prio() == 256)
		&& (_kernel.ready_q.cache != _current)) {

		(void)z_swap_irqlock(irq_lock);
	}
}

/**
 * Thru this function the IRQ controller can raise an immediate  interrupt which
 * will interrupt the SW itself
 * (this function should only be called from the HW model code, from SW threads)
 */
void posix_irq_handler_im_from_sw(void)
{
	/*
	 * if a higher priority interrupt than the possibly currently running is
	 * pending we go immediately into irq_handler() to vector into its
	 * handler
	 */
	if (hw_irq_ctrl_get_highest_prio_irq() != -1) {
		if (!posix_is_cpu_running()) { /* LCOV_EXCL_BR_LINE */
			/* LCOV_EXCL_START */
			posix_print_error_and_exit("programming error: %s "
					"called from a HW model thread\n",
					__func__);
			/* LCOV_EXCL_STOP */
		}
		posix_irq_handler();
	}
}












/**
 * Similar to ARM's NVIC_SetPendingIRQ
 * set a pending IRQ from SW
 *
 * Note that this will interrupt immediately if the interrupt is not masked and
 * IRQs are not locked, and this interrupt has higher priority than a possibly
 * currently running interrupt
 */
void posix_sw_set_pending_IRQ(unsigned int IRQn)
{
	hw_irq_ctrl_raise_im_from_sw(IRQn);
}

/**
 * Similar to ARM's NVIC_ClearPendingIRQ
 * clear a pending irq from SW
 */
void posix_sw_clear_pending_IRQ(unsigned int IRQn)
{
	hw_irq_ctrl_clear_irq(IRQn);
}

