/****************************************************************************
 *
 * posic_soc_if.cpp
 *
 *  Created on: Dec 17, 2021
 *      Author: mballance
 *
 ****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <arch/posix/posix_soc_if.h>
#include "ZephyrCosim.h"

/**
 * Normally called from arch_cpu_idle():
 *   the idle loop will call this function to set the CPU to "sleep".
 * Others may also call this function with care. The CPU will be set to sleep
 * until some interrupt awakes it.
 * Interrupts should be enabled before calling.
 */
void posix_halt_cpu(void) {
	ZephyrCosim::inst()->halt_cpu();
}

/**
 * Implementation of arch_cpu_atomic_idle() for this SOC
 */
void posix_atomic_halt_cpu(unsigned int imask) {
	ZephyrCosim::inst()->atomic_halt_cpu(imask);
}

void posix_irq_enable(unsigned int irq) {
	ZephyrCosim::inst()->enable_irq(irq);
}

void posix_irq_disable(unsigned int irq) {
	ZephyrCosim::inst()->disable_irq(irq);
}

int posix_irq_is_enabled(unsigned int irq) {
	return ZephyrCosim::inst()->irq_enabled(irq);
}

/**
 * @brief Disable all interrupts on the CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API. It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 */
unsigned int posix_irq_lock(void) {
	return ZephyrCosim::inst()->lock_irq();
}

void posix_irq_unlock(unsigned int key) {
	ZephyrCosim::inst()->unlock_irq(key);
}

/**
 *
 * @brief Enable all interrupts on the CPU
 *
 * This routine re-enables interrupts on the CPU.  The @a key parameter is a
 * board-dependent lock-out key that is returned by a previous invocation of
 * board_irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @return N/A
 *
 */
void posix_irq_full_unlock(void) {
	ZephyrCosim::inst()->irq_full_unlock();
}

int posix_get_current_irq(void) {
	return ZephyrCosim::inst()->running_irq();
}

#ifdef CONFIG_IRQ_OFFLOAD
/**
 * Storage for functions offloaded to IRQ
 */
static void (*off_routine)(const void *);
static const void *off_parameter;

/**
 * IRQ handler for the SW interrupt assigned to irq_offload()
 */
static void offload_sw_irq_handler(const void *a)
{
	ARG_UNUSED(a);
	off_routine(off_parameter);
}

/**
 * @brief Run a function in interrupt context
 *
 * Raise the SW IRQ assigned to handled this
 */
void posix_irq_offload(void (*routine)(const void *), const void *parameter)
{
	off_routine = routine;
	off_parameter = parameter;
	posix_isr_declare(OFFLOAD_SW_IRQ, 0, offload_sw_irq_handler, NULL);
	posix_irq_enable(OFFLOAD_SW_IRQ);
	posix_sw_set_pending_IRQ(OFFLOAD_SW_IRQ);
	posix_irq_disable(OFFLOAD_SW_IRQ);
}
#endif /* CONFIG_IRQ_OFFLOAD */

extern "C" void posix_exit(int exit_code) {
	static int max_exit_code;

	fprintf(stdout, "posix_exit\n");

	max_exit_code = (exit_code>max_exit_code)?exit_code:max_exit_code;
	/*
	 * posix_soc_clean_up may not return if this is called from a SW thread,
	 * but instead it would get posix_exit() recalled again
	 * ASAP from the HW thread
	 */
//	posix_soc_clean_up();

	ZephyrCosim::inst()->exit(exit_code);

//	hwm_cleanup();
//	native_cleanup_cmd_line();
	exit(max_exit_code);
}

