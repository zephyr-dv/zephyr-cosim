/*
 * ZephyrCosim.h
 *
 *  Created on: Dec 17, 2021
 *      Author: mballance
 */

#pragma once
#include <stdint.h>
#include <pthread.h>
#include "sw_isr_table.h"
#include "tblink_rpc/IEndpoint.h"

class ZephyrCosim {
public:
	ZephyrCosim();

	virtual ~ZephyrCosim();

	int run(int argc, char **argv);

	void exit(int32_t code);

	static ZephyrCosim *inst();

	void isr_declare(
		uint32_t		irq_p,
		int32_t			flags,
		void 			isr_p(const void *),
		const void 		*isr_param_p);

	// Interrupt Controller API
	void irq_prio(
			uint32_t		irq,
			uint32_t		prio,
			uint32_t		flags);

	void enable_irq(uint32_t irq);

	bool irq_enabled(uint32_t irq) { return (m_irq_mask & (1ULL << irq)); }

	void disable_irq(uint32_t irq);

	uint32_t lock_irq();

	void irq_full_unlock();

	void unlock_irq(uint32_t mask);

	int32_t running_irq();

	void halt_cpu();

	void atomic_halt_cpu(uint32_t mask);

private:

	int init_cosim(int argc, char **argv);

	void boot_cpu();

	void irq_handler();

	static void *thread_w(void *);

	void run_native_tasks(int level);

	static void *message_processing_thread_w(void *);

	void message_processing_thread();

private:
	static const int N_IRQS = 32;

	static ZephyrCosim			*m_inst;

	tblink_rpc_core::IEndpoint		*m_ep;

	pthread_t						m_zephyr_thread;
	pthread_mutex_t					m_mtx_cpu;
	pthread_cond_t					m_cond_cpu;
	bool							m_cpu_halted;
	bool							m_soc_terminate;

	pthread_t						m_messages_processing_thread;

	using isr_table_entry_t = struct _isr_list;
	isr_table_entry_t				m_irq_vector_table[N_IRQS];

	// Interrupt Controller Data
	uint8_t							m_irq_prio[N_IRQS];
	uint64_t						m_irq_status;		// Pending interrupts
	uint64_t						m_irq_premask;		// Interrupts before masking
	uint64_t						m_irq_mask;

	bool							m_irqs_locked;
	bool							m_irq_lock_ignore;

};

