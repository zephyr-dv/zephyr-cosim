/*
 * ZephyrCosim.cpp
 *
 *  Created on: Dec 17, 2021
 *      Author: mballance
 */

#include <stdio.h>
#include <string.h>
#include "ZephyrCosim.h"
#include "kernel_internal.h"
#include "posix_arch_internal.h"
#include <arch/posix/posix_soc_if.h>
#include "posix_core.h"
#include "soc.h"
#include "tblink_rpc/ITbLink.h"
#include "tblink_rpc/loader.h"
#include "zephyr_cosim_sysrw.h"

ZephyrCosim::ZephyrCosim() {
	m_ep = 0;
	m_zephyr_thread = 0;
	m_messages_processing_thread = 0;

	m_cpu_halted = false;
	m_soc_terminate = false;

	memset(&m_irq_vector_table, 0, sizeof(m_irq_vector_table));

	memset(&m_irq_prio, 255, sizeof(m_irq_prio));
	m_irq_status = 0;
	m_irq_mask = 0;
	m_irq_premask = 0;
	m_irqs_locked = false;
	m_irq_lock_ignore = false;
}

ZephyrCosim::~ZephyrCosim() {
	// TODO Auto-generated destructor stub
}

int ZephyrCosim::run(int argc, char **argv) {
	if (init_cosim(argc, argv) == -1) {
		fprintf(stdout, "Error: failed to initialize cosim link\n");
		return 1;
	}

	// Start up the message-processing thread
	if (pthread_create(&m_messages_processing_thread, 0,
			&ZephyrCosim::message_processing_thread_w, this) == -1) {
		fprintf(stdout, "Zephyr-Cosim Error: failed to create pthread\n");
		return -1;
	}

	run_native_tasks(_NATIVE_PRE_BOOT_1_LEVEL);

//	native_handle_cmd_line(argc, argv);

	run_native_tasks(_NATIVE_PRE_BOOT_2_LEVEL);

//	hwm_init();

	run_native_tasks(_NATIVE_PRE_BOOT_3_LEVEL);

	fprintf(stdout, "--> posix_boot_cpu\n");
	fflush(stdout);
	boot_cpu();
	fprintf(stdout, "<-- posix_boot_cpu\n");
	fflush(stdout);

	fprintf(stdout, "--> NATIVE_FIRST_SLEEP_LEVEL\n");
	fflush(stdout);
	run_native_tasks(_NATIVE_FIRST_SLEEP_LEVEL);
	fprintf(stdout, "<-- NATIVE_FIRST_SLEEP_LEVEL\n");
	fflush(stdout);

	// Drop through to here once we WFI

#ifdef UNDEFINED
	zephyr_cosim_release();

	fprintf(stdout, "--> Wait for soc_terminate\n");
	fflush(stdout);
	while (!soc_terminate) {
		int rv;

		// Run states
		// - Completely idle with no memory accesses pending
		// -> If pending interrupts, issue them
		// -> Else, release peer to run
		// - Memory accesses pending
		// -> Process messages
		// -> If IRQs are received, queue them
		//
		// Submit
		// Instruct other side to release

		// Wait for event

		//
		fprintf(stdout, "--> zephyr_cosim_process()\n");
		fflush(stdout);
		rv = zephyr_cosim_process();
		fprintf(stdout, "<-- zephyr_cosim_process() %d\n", rv);
		fflush(stdout);

		if (rv == -1) {
			posix_exit(0);
		}

		// TODO: process_one_message()
//		pthread_cond_wait(&cond_cpu, &mtx_cpu);
	}
	fprintf(stdout, "<-- Wait for soc_terminate\n");
	fflush(stdout);

	zephyr_cosim_exit(0);
//	hwm_main_loop();
#endif /* UNDEFINED */

	/* This line should be unreachable */
	return 1; /* LCOV_EXCL_LINE */
}

ZephyrCosim *ZephyrCosim::inst() {
	if (!m_inst) {
		m_inst = new ZephyrCosim();
	}
	return m_inst;
}

void ZephyrCosim::isr_declare(
		unsigned int irq_p,
		int flags,
		void isr_p(const void *),
		const void *isr_param_p) {
	m_irq_vector_table[irq_p].irq   = irq_p;
	m_irq_vector_table[irq_p].func  = reinterpret_cast<void *>(isr_p);
	m_irq_vector_table[irq_p].param = isr_param_p;
	m_irq_vector_table[irq_p].flags = flags;
}

void ZephyrCosim::irq_prio(
			uint32_t		irq,
			uint32_t		prio,
			uint32_t		flags) {
	m_irq_prio[irq] = prio;
}

void ZephyrCosim::enable_irq(uint32_t irq) {
	m_irq_mask |= 1ULL << irq;

	if (m_irq_premask & (1ULL << irq)) {
		// TODO:
		// hw_irq_ctrl_raise_imm_from_sw(irq);
	}
}

void ZephyrCosim::disable_irq(uint32_t irq) {
	m_irq_mask |= ~(1ULL << irq);
}

uint32_t ZephyrCosim::lock_irq() {
	// TODO:
//	return hw_irq_ctrl_change_lock(true);
	return 0;
}

void ZephyrCosim::irq_full_unlock() {
	// TODO:
//	hw_irq_ctrl_change_lock(false);
}

void ZephyrCosim::unlock_irq(uint32_t key) {
	// TODO:
//	hw_irq_ctrl_change_lock(key);
}

int32_t ZephyrCosim::running_irq() {
	// TODO:
	return -1;
}

void ZephyrCosim::halt_cpu() {
	fprintf(stdout, "--> posix_halt_cpu\n");
	fflush(stdout);

	/*
	 * We set the CPU in the halted state (this blocks this pthread
	 * until the CPU is awoken again by the HW models)
	 */
	// TODO:
//	ZephyrCosim::inst()->change_cpu_state_and_wait(true);

	/* We are awoken, normally that means some interrupt has just come
	 * => let the "irq handler" check if/what interrupt was raised
	 * and call the appropriate irq handler.
	 *
	 * Note that, the interrupt handling may trigger a arch_swap() to
	 * another Zephyr thread. When posix_irq_handler() returns, the Zephyr
	 * kernel has swapped back to this thread again
	 */
	irq_handler();

	/*
	 * And we go back to whatever Zephyr thread calleed us.
	 */
}

void ZephyrCosim::atomic_halt_cpu(uint32_t mask) {
	irq_full_unlock();
	halt_cpu();
	irq_unlock(mask);
}

int ZephyrCosim::init_cosim(int argc, char **argv) {
	std::string tblink_rpc_lib;

	fprintf(stdout, "--> zephyr_cosim_init\n");
	fflush(stdout);

	for (uint32_t i=0; i<argc; i++) {
		fprintf(stdout, "Arg[%d] %s\n", i, argv[i]);
	}

	if (argc != 2) {
		fprintf(stdout, "Error: no tblink_rpc_lib specified\n");
		return -1;
	}

	tblink_rpc_lib = argv[1];

	tblink_rpc_core::ITbLink *tblink = get_tblink(tblink_rpc_lib.c_str());
	tblink_rpc_core::ILaunchType *launch = tblink->findLaunchType("connect.socket");

	fprintf(stdout, "launch=%p\n", launch);
	tblink_rpc_core::ILaunchParams *params = launch->newLaunchParams();
	params->add_param("host", getenv("TBLINK_HOST"));
	params->add_param("port", getenv("TBLINK_PORT"));

	tblink_rpc_core::ILaunchType::result_t result = launch->launch(params, 0);

	if (!result.first) {
		fprintf(stdout, "Failed: %s\n", result.second.c_str());
		return -1;
	} else {
		fprintf(stdout, "Succeeded\n");
	}

//	EndpointServicesZephyrCosim *services = new EndpointServicesZephyrCosim();
	m_ep = result.first;
	/*
	if (m_ep->init(services, 0) == -1) {
		fprintf(stdout, "Initialization failed: %s\n", result.first->last_error().c_str());
		return -1;
	}
	 */

	int rv;

//	prv_cosim_if = new ZephyrCosimIf(prv_endpoint);

	while ((rv=m_ep->is_init()) == 0) {
		if (m_ep->process_one_message() == -1) {
			fprintf(stdout, "Error: failed waiting for init phase\n");
			return -1;
		}
	}

	if (rv == -1) {
		fprintf(stdout, "ZephyrCosim Error: init failed\n");
		return -1;
	}

	/* TODO:
		std::vector<std::string> args = prv_endpoint->args();
		for (std::vector<std::string>::const_iterator
				it=args.begin();
				it!=args.end(); it++) {
			fprintf(stdout, "Arg: %s\n", it->c_str());
		}
	 */

	// TODO: register API type and inst

	fprintf(stdout, "--> build_complete\n");
	fflush(stdout);
	if (m_ep->build_complete() == -1) {
		fprintf(stdout, "Error: build phase failed: %s\n", m_ep->last_error().c_str());
		return -1;
	}

	while ((rv=m_ep->is_build_complete()) == 0) {
		if (m_ep->process_one_message() == -1) {
			fprintf(stdout, "Error: failed waiting for build phase completion\n");
			return -1;
		}
	}

	if (rv == -1) {
		fprintf(stdout, "ZephyrCosim Error: build failed\n");
		return -1;
	}

	fprintf(stdout, "<-- build_complete\n");
	fflush(stdout);

	fprintf(stdout, "--> connect_complete\n");
	fflush(stdout);
	if (m_ep->connect_complete() == -1) {
		fprintf(stdout, "Error: connect phase failed: %s\n", m_ep->last_error().c_str());
		return -1;
	}

	while ((rv=m_ep->is_connect_complete()) == 0) {
		if (m_ep->process_one_message() == -1) {
			fprintf(stdout, "Error: failed waiting for connect phase completion\n");
			return -1;
		}
	}

	if (rv == -1) {
		fprintf(stdout, "ZephyrCosim Error: connect failed\n");
		return -1;
	}

	fprintf(stdout, "<-- connect_complete\n");
	fflush(stdout);

	fprintf(stdout, "<-- zephyr_cosim_init\n");
	fflush(stdout);

	return 0;
}

void ZephyrCosim::exit(int32_t code) {

}

void ZephyrCosim::boot_cpu() {
	PC_SAFE_CALL(pthread_mutex_lock(&m_mtx_cpu));

	m_cpu_halted = false;

	pthread_t zephyr_thread;

	/* Create a thread for Zephyr init: */
	PC_SAFE_CALL(pthread_create(
			&m_zephyr_thread,
			NULL,
			&ZephyrCosim::thread_w, this));

	/* And we wait until Zephyr has run til completion (has gone to idle) */
	while (m_cpu_halted == false) {
		pthread_cond_wait(&m_cond_cpu, &m_mtx_cpu);
	}
	PC_SAFE_CALL(pthread_mutex_unlock(&m_mtx_cpu));

	if (m_soc_terminate) {
		exit(0);
	}
}

void ZephyrCosim::irq_handler() {
#ifdef UNDEFINED
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
#endif
}

void *ZephyrCosim::thread_w(void *zc_p) {
	ZephyrCosim *zc = reinterpret_cast<ZephyrCosim *>(zc_p);
	/* Ensure posix_boot_cpu has reached the cond loop */
	PC_SAFE_CALL(pthread_mutex_lock(&zc->m_mtx_cpu));
	PC_SAFE_CALL(pthread_mutex_unlock(&zc->m_mtx_cpu));

#if (POSIX_ARCH_SOC_DEBUG_PRINTS)
		pthread_t zephyr_thread = pthread_self();

		PS_DEBUG("Zephyr init started (%lu)\n",
			zephyr_thread);
#endif

	posix_init_multithreading();

	/* Start Zephyr: */
	z_cstart();
	CODE_UNREACHABLE;

	return 0;
}

void *ZephyrCosim::message_processing_thread_w(void *zc_p) {
	reinterpret_cast<ZephyrCosim *>(zc_p)->message_processing_thread();
	return 0;
}

void ZephyrCosim::message_processing_thread() {
	// TODO: condition on 'running' ?
	while (m_ep->process_one_message() != -1) {
		;
	}
}

/**
 * @brief Run the set of special native tasks corresponding to the given level
 *
 * @param level One of _NATIVE_*_LEVEL as defined in soc.h
 */
void ZephyrCosim::run_native_tasks(int level)
{
	extern void (*__native_PRE_BOOT_1_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_2_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_3_tasks_start[])(void);
	extern void (*__native_FIRST_SLEEP_tasks_start[])(void);
	extern void (*__native_ON_EXIT_tasks_start[])(void);
	extern void (*__native_tasks_end[])(void);

	static void (**native_pre_tasks[])(void) = {
		__native_PRE_BOOT_1_tasks_start,
		__native_PRE_BOOT_2_tasks_start,
		__native_PRE_BOOT_3_tasks_start,
		__native_FIRST_SLEEP_tasks_start,
		__native_ON_EXIT_tasks_start,
		__native_tasks_end
	};

	void (**fptr)(void);

	for (fptr = native_pre_tasks[level]; fptr < native_pre_tasks[level+1];
		fptr++) {
		if (*fptr) { /* LCOV_EXCL_BR_LINE */
			(*fptr)();
		}
	}
}

ZephyrCosim *ZephyrCosim::m_inst = 0;



// TODO: Need functions from 'posix_soc_if.h'

// TODO: Need implementation of functions from 'zephyr_cosim_sysrw .h'

// TODO: need a 'main' function
