/*
 * ZephyrCosim.cpp
 *
 *  Created on: Dec 17, 2021
 *      Author: mballance
 */

#include <stdio.h>
#include <string.h>
#include "EndpointServicesZephyrCosim.h"
#include "ZephyrCosim.h"
#include "kernel_internal.h"
#include "kswap.h"
#include "posix_arch_internal.h"
#include <arch/posix/posix_soc_if.h>
#include "kernel_internal.h"
#include "posix_core.h"
#include "soc.h"
#include "tblink_rpc/ITbLink.h"
#include "tblink_rpc/IParamValVec.h"
#include "tblink_rpc/loader.h"
#include "zephyr_cosim_sysrw.h"

#define EN_DEBUG_ZEPHYR_COSIM

#ifdef EN_DEBUG_ZEPHYR_COSIM
#define DEBUG_ENTER(fmt, ...) fprintf(stdout, "--> ZephyrCosim::" fmt "\n", ##__VA_ARGS__); fflush(stdout)
#define DEBUG_LEAVE(fmt, ...) fprintf(stdout, "<--ZephyrCosim::" fmt "\n", ##__VA_ARGS__); fflush(stdout)
#define DEBUG(fmt, ...) fprintf(stdout, "ZephyrCosim: " fmt "\n", ##__VA_ARGS__); fflush(stdout)
#else
#define DEBUG_ENTER(fmt, ...)
#define DEBUG_LEAVE(fmt, ...)
#define DEBUG(fmt, ...)
#endif

using namespace tblink_rpc_core;


ZephyrCosim::ZephyrCosim() {
	m_ep = 0;
	m_zephyr_thread = 0;
	m_messages_processing_thread = 0;

	m_cpu_halted = false;
	m_soc_terminate = false;

	m_ifinst = 0;
	m_read8 = 0;
	m_write8 = 0;
	m_read16 = 0;
	m_write16 = 0;
	m_read32 = 0;
	m_write32 = 0;
	m_sys_irq = 0;

	memset(&m_irq_vector_table, 0, sizeof(m_irq_vector_table));

	memset(&m_irq_prio, 255, sizeof(m_irq_prio));
	m_irq_status = 0;
	m_irq_mask = 0;
	m_running_irq = -1;
	m_irq_premask = 0;
	m_irqs_locked = false;
	m_may_swap = false;
	m_irq_lock_ignore = false;
}

ZephyrCosim::~ZephyrCosim() {
	// TODO Auto-generated destructor stub
}

int ZephyrCosim::run(int argc, char **argv) {
	DEBUG_ENTER("run");

	pthread_mutex_init(&m_mtx_invoke, 0);
	pthread_cond_init(&m_cond_invoke, 0);

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
	DEBUG("Note: Execution has suspended for the first time");

	pthread_mutex_lock(&m_mtx_cpu);
	while (!m_soc_terminate) {
		DEBUG_ENTER("run::wait 0x%08x", z_main_thread.base.user_options);
		pthread_cond_wait(&m_cond_cpu, &m_mtx_cpu);
		DEBUG_LEAVE("run::wait %d 0x%08x", m_soc_terminate, z_main_thread.base.user_options);
	}
	pthread_mutex_unlock(&m_mtx_cpu);

	DEBUG("Note: m_soc_terminate=%d", m_soc_terminate);

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

	DEBUG_LEAVE("run");
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
	DEBUG_ENTER("enable_irq %d", irq);
	m_irq_mask |= 1ULL << irq;

	if (m_irq_premask & (1ULL << irq)) {
		// TODO:
		// hw_irq_ctrl_raise_imm_from_sw(irq);
	}
	DEBUG_LEAVE("enable_irq %d", irq);
}

bool ZephyrCosim::irq_enabled(uint32_t irq) {
	DEBUG_ENTER("irq_enabled %d", irq);
	bool ret = (m_irq_mask & (1ULL << irq));
	DEBUG_LEAVE("irq_enabled %d %d", irq, ret);
	return ret;
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
	DEBUG_ENTER("running_irq");
	DEBUG_LEAVE("running_irq %d", m_running_irq);
	return m_running_irq;
}

void ZephyrCosim::halt_cpu() {
	DEBUG_ENTER("halt_cpu main_thread=0x%08x", z_main_thread.base.user_options);

	if (!(z_main_thread.base.user_options & K_ESSENTIAL)) {
		// The main thread has exited. Notify the connected
		// simulator that we are complete
		m_soc_terminate = true;

		pthread_mutex_lock(&m_mtx_cpu);
		m_cpu_halted = true;
		pthread_cond_broadcast(&m_cond_cpu);

		while (m_cpu_halted) {
			DEBUG_ENTER("halt_cpu::cond_wait");
			pthread_cond_wait(&m_cond_cpu, &m_mtx_cpu);
			DEBUG_LEAVE("halt_cpu::cond_wait %d", m_cpu_halted);
		}
		pthread_mutex_unlock(&m_mtx_cpu);
	} else {
		// Main thread is still running. Release the simulation
		// to run until an event is received.

		/*
		 * We set the CPU in the halted state (this blocks this pthread
		 * until the CPU is awakened via an interrupt
		 */
		pthread_mutex_lock(&m_mtx_cpu);

		DEBUG_ENTER("halt_cpu::release");
		m_ep->update_comm_mode(IEndpoint::Automatic, IEndpoint::Released);
		DEBUG_LEAVE("halt_cpu::release");

		m_cpu_halted = true;

		while (m_cpu_halted) {
			DEBUG_ENTER("halt_cpu::cond_wait");
			pthread_cond_wait(&m_cond_cpu, &m_mtx_cpu);
			DEBUG_LEAVE("halt_cpu::cond_wait %d", m_cpu_halted);
		}

		pthread_mutex_unlock(&m_mtx_cpu);

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
		DEBUG_ENTER("halt_cpu::irq_handler");
		irq_handler();
		DEBUG_LEAVE("halt_cpu::irq_handler");
	}

	/*
	 * And we go back to whatever Zephyr thread calleed us.
	 */
	DEBUG_LEAVE("halt_cpu");
}

void ZephyrCosim::atomic_halt_cpu(uint32_t mask) {
	irq_full_unlock();
	halt_cpu();
	irq_unlock(mask);
}

uint8_t ZephyrCosim::read8(mem_addr_t addr) {
	DEBUG_ENTER("read8");

	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(addr, 64));

	bool complete = false;
	uint8_t ret = 0;

	m_ifinst->invoke_nb(
			m_read8,
			params,
			[&](IParamVal *retval) {
				DEBUG_ENTER("read8::rsp");
				pthread_mutex_lock(&m_mtx_invoke);
				complete = true;
				ret = dynamic_cast<IParamValInt *>(retval)->val_u();
				pthread_cond_broadcast(&m_cond_invoke);
				pthread_mutex_unlock(&m_mtx_invoke);
				DEBUG_LEAVE("read8::rsp");
			});

	pthread_mutex_lock(&m_mtx_invoke);
	while (!complete) {
		pthread_cond_wait(&m_cond_invoke, &m_mtx_invoke);
	}
	pthread_mutex_unlock(&m_mtx_invoke);

	DEBUG_LEAVE("read8");
	return ret;
}

void ZephyrCosim::write8(uint8_t data, mem_addr_t addr) {
	DEBUG_ENTER("write8");
	// TODO: should acquire lock here

	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(data, 32));
	params->push_back(m_ifinst->mkValIntU(addr, 64));

	bool complete = false;

	m_ifinst->invoke_nb(
			m_write8,
			params,
			[&](IParamVal *retval) {
				DEBUG_ENTER("write8::rsp");
				pthread_mutex_lock(&m_mtx_invoke);
				complete = true;
				pthread_cond_broadcast(&m_cond_invoke);
				pthread_mutex_unlock(&m_mtx_invoke);
				DEBUG_LEAVE("write8::rsp");
			});

	pthread_mutex_lock(&m_mtx_invoke);
	while (!complete) {
		pthread_cond_wait(&m_cond_invoke, &m_mtx_invoke);
	}
	pthread_mutex_unlock(&m_mtx_invoke);

	DEBUG_LEAVE("write8");
}

uint16_t ZephyrCosim::read16(mem_addr_t addr) {
	DEBUG_ENTER("read16");

	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(addr, 64));

	bool complete = false;
	uint16_t ret = 0;

	m_ifinst->invoke_nb(
			m_read16,
			params,
			[&](IParamVal *retval) {
				DEBUG_ENTER("read16::rsp");
				pthread_mutex_lock(&m_mtx_invoke);
				complete = true;
				ret = dynamic_cast<IParamValInt *>(retval)->val_u();
				pthread_cond_broadcast(&m_cond_invoke);
				pthread_mutex_unlock(&m_mtx_invoke);
				DEBUG_LEAVE("read16::rsp");
			});

	pthread_mutex_lock(&m_mtx_invoke);
	while (!complete) {
		pthread_cond_wait(&m_cond_invoke, &m_mtx_invoke);
	}
	pthread_mutex_unlock(&m_mtx_invoke);

	DEBUG_LEAVE("read16");
	return ret;
}

void ZephyrCosim::write16(uint16_t data, mem_addr_t addr) {
	DEBUG_ENTER("write16");
	// TODO: should acquire lock here

	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(data, 32));
	params->push_back(m_ifinst->mkValIntU(addr, 64));

	bool complete = false;

	m_ifinst->invoke_nb(
			m_write16,
			params,
			[&](IParamVal *retval) {
				DEBUG_ENTER("write16::rsp");
				pthread_mutex_lock(&m_mtx_invoke);
				complete = true;
				pthread_cond_broadcast(&m_cond_invoke);
				pthread_mutex_unlock(&m_mtx_invoke);
				DEBUG_LEAVE("write16::rsp");
			});

	pthread_mutex_lock(&m_mtx_invoke);
	while (!complete) {
		pthread_cond_wait(&m_cond_invoke, &m_mtx_invoke);
	}
	pthread_mutex_unlock(&m_mtx_invoke);

	DEBUG_LEAVE("write16");
}

uint32_t ZephyrCosim::read32(mem_addr_t addr) {
	DEBUG_ENTER("read32");

	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(addr, 64));

	bool complete = false;
	uint32_t ret = 0;

	m_ifinst->invoke_nb(
			m_read32,
			params,
			[&](IParamVal *retval) {
				DEBUG_ENTER("read32::rsp");
				pthread_mutex_lock(&m_mtx_invoke);
				complete = true;
				ret = dynamic_cast<IParamValInt *>(retval)->val_u();
				pthread_cond_broadcast(&m_cond_invoke);
				pthread_mutex_unlock(&m_mtx_invoke);
				DEBUG_LEAVE("read32::rsp");
			});

	pthread_mutex_lock(&m_mtx_invoke);
	while (!complete) {
		pthread_cond_wait(&m_cond_invoke, &m_mtx_invoke);
	}
	pthread_mutex_unlock(&m_mtx_invoke);

	DEBUG_LEAVE("read32");
	return ret;
}

void ZephyrCosim::write32(uint32_t data, mem_addr_t addr) {
	DEBUG_ENTER("write32");
	// TODO: should acquire lock here

	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(data, 32));
	params->push_back(m_ifinst->mkValIntU(addr, 64));

	bool complete = false;

	m_ifinst->invoke_nb(
			m_write32,
			params,
			[&](IParamVal *retval) {
				DEBUG_ENTER("write32::rsp");
				pthread_mutex_lock(&m_mtx_invoke);
				complete = true;
				pthread_cond_broadcast(&m_cond_invoke);
				pthread_mutex_unlock(&m_mtx_invoke);
				DEBUG_LEAVE("write32::rsp");
			});

	pthread_mutex_lock(&m_mtx_invoke);
	while (!complete) {
		pthread_cond_wait(&m_cond_invoke, &m_mtx_invoke);
	}
	pthread_mutex_unlock(&m_mtx_invoke);

	DEBUG_LEAVE("write32");
}

int ZephyrCosim::init_cosim(int argc, char **argv) {
	DEBUG_ENTER("init_cosim");
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

	EndpointServicesZephyrCosim *services = new EndpointServicesZephyrCosim();
	m_ep = result.first;

	IInterfaceType *iftype = defineIftype();

	m_ifinst = m_ep->defineInterfaceInst(
			iftype,
			"cosim",
			true,
			std::bind(&ZephyrCosim::req_invoke,
					this,
					std::placeholders::_1,
					std::placeholders::_2,
					std::placeholders::_3,
					std::placeholders::_4));

	if (m_ep->init(services, 0) == -1) {
		fprintf(stdout, "Initialization failed: %s\n", result.first->last_error().c_str());
		return -1;
	}

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

	DEBUG_LEAVE("init_cosim");
	return 0;
}

void ZephyrCosim::exit(int32_t code) {
	DEBUG_ENTER("exit %d", code);

	DEBUG_LEAVE("exit %d", code);
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
	DEBUG_ENTER("irq_handler");

	// TODO: what locking is needed here?

	bool irqs_locked = m_irqs_locked;

	if (irqs_locked) {
		return;
	}

	if (_kernel.cpus[0].nested == 0) {
		m_may_swap = false;
	}

	_kernel.cpus[0].nested++;

	int irq_num;
	while ((irq_num=next_pending_irq()) != -1) {
		DEBUG_ENTER("irq_handler::irq %d", irq_num);

		int32_t last_running_irq = m_running_irq;

		// Acknowledge the pending-interrupt flag
		m_irq_status &= ~(1ULL << irq_num);
		m_irq_premask &= ~(1ULL << irq_num);

//		sys_trace_isr_enter();

		m_running_irq = irq_num;
		if (m_irq_vector_table[irq_num].func == NULL) { /* LCOV_EXCL_BR_LINE */
			/* LCOV_EXCL_START */
			DEBUG("No function registered for IRQ %d", irq_num);
			posix_print_error_and_exit("Received irq %i without a "
						"registered handler\n",
						irq_num);
			/* LCOV_EXCL_STOP */
		} else {
			if (m_irq_vector_table[irq_num].flags & ISR_FLAG_DIRECT) {
				DEBUG("Making a direct ISR call");
				m_may_swap |= ((int (*)())m_irq_vector_table[irq_num].func)();
			} else {
	#ifdef CONFIG_PM
				posix_irq_check_idle_exit();
	#endif
				DEBUG("Making a normal ISR call");
				((void (*)(const void *))m_irq_vector_table[irq_num].func)
						(m_irq_vector_table[irq_num].param);
				m_may_swap = true;
			}
		}
		m_running_irq = last_running_irq;

//		sys_trace_isr_exit();
		DEBUG_LEAVE("irq_handler::irq %d", irq_num);
	}

	_kernel.cpus[0].nested--;

	/* Call swap if all the following is true:
	 * 1) may_swap was enabled
	 * 2) We are not nesting irq_handler calls (interrupts)
	 * 3) Next thread to run in the ready queue is not this thread
	 */
	if (m_may_swap
		/*&& (hw_irq_ctrl_get_cur_prio() == 256) */
		&& (_kernel.ready_q.cache != _current)) {

		DEBUG_ENTER("irq_handler::swap");
		(void)z_swap_irqlock((uint32_t)irqs_locked);
		DEBUG_LEAVE("irq_handler::swap");
	}
	DEBUG_LEAVE("irq_handler");

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

int32_t ZephyrCosim::next_pending_irq() {
	DEBUG_ENTER("next_pending_irq: status=0x%08x", m_irq_status);
	if (m_irqs_locked) {
		DEBUG_LEAVE("next_pending_irq -- interrupts locked");
		return -1;
	}

	int32_t winner = -1;
	int32_t winner_prio = 256;

	int32_t currently_running_prio = 10000;
	uint32_t irq_num = 0;
	uint32_t irq_status = m_irq_status;
	while (irq_status) {
		while (!(irq_status&1)) {
			irq_status >>= 1;
			irq_num += 1;
		}

		if ((winner_prio > (int)m_irq_prio[irq_num]) &&
				(currently_running_prio > (int)m_irq_prio[irq_num])) {
			winner = irq_num;
			winner_prio = m_irq_prio[irq_num];
		}
		irq_status >>= 1;
	}

	DEBUG_LEAVE("next_pending_irq: status=0x%08x %d", m_irq_status, winner);
	return winner;
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
	DEBUG_ENTER("message_processing_thread");
	// TODO: condition on 'running' ?
	while (m_ep->process_one_message() != -1) {
		;
	}
	DEBUG_LEAVE("message_processing_thread");
}

void ZephyrCosim::req_invoke(
	tblink_rpc_core::IInterfaceInst		*ifinst,
	tblink_rpc_core::IMethodType		*method,
	intptr_t							call_id,
	tblink_rpc_core::IParamValVec		*params) {
	DEBUG_ENTER("req_invoke");
	if (method == m_sys_irq) {
		DEBUG("Received IRQ");

		DEBUG_ENTER("req_invoke::rsp");
		pthread_mutex_lock(&m_mtx_invoke);
		// Send an ack to the interrupt
		ifinst->invoke_rsp(call_id, 0);
		pthread_mutex_unlock(&m_mtx_invoke);
		DEBUG_LEAVE("req_invoke::rsp");

		DEBUG_ENTER("req_invoke::wake_cpu");
		pthread_mutex_lock(&m_mtx_cpu);
		// TODO: update interrupt-controller state
		m_cpu_halted = false;
		m_irq_status |= 1;
		pthread_cond_broadcast(&m_cond_cpu);
		pthread_mutex_unlock(&m_mtx_cpu);
		DEBUG_LEAVE("req_invoke::wake_cpu");
	} else {
		fprintf(stdout, "ZephyrCosim Error: unknown method invocation\n");
		fflush(stdout);
	}

	DEBUG_LEAVE("req_invoke");
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

tblink_rpc_core::IInterfaceType *ZephyrCosim::defineIftype() {
	IInterfaceType *iftype = m_ep->findInterfaceType("zephyr_cosim_if");

	if (!iftype) {
		IInterfaceTypeBuilder *iftype_b = m_ep->newInterfaceTypeBuilder(
				"zephyr_cosim_if");
		IMethodTypeBuilder *method_b;

		// read8
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_read8",
				1,
				iftype_b->mkTypeInt(false, 8),
				true,
				true);
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		m_read8 = iftype_b->add_method(method_b);

		// write8
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_write8",
				2,
				0,
				true,
				true);
		method_b->add_param("data",
				iftype_b->mkTypeInt(false, 8));
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		m_write8 = iftype_b->add_method(method_b);


		// read16
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_read16",
				3,
				iftype_b->mkTypeInt(false, 16),
				true,
				true);
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		m_read16 = iftype_b->add_method(method_b);

		// write16
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_write16",
				4,
				0,
				true,
				true);
		method_b->add_param("data",
				iftype_b->mkTypeInt(false, 16));
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		m_write16 = iftype_b->add_method(method_b);

		// read32
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_read32",
				5,
				iftype_b->mkTypeInt(false, 32),
				true,
				true);
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		m_read32 = iftype_b->add_method(method_b);

		// write32
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_write32",
				6,
				0,
				true,
				true);
		method_b->add_param("data",
				iftype_b->mkTypeInt(false, 32));
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		m_write32 = iftype_b->add_method(method_b);

		// irq
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_irq",
				7,
				0,
				false,
				false);
		method_b->add_param("num",
				iftype_b->mkTypeInt(false, 8));
		m_sys_irq = iftype_b->add_method(method_b);

		iftype = m_ep->defineInterfaceType(iftype_b);
	}

	return iftype;
	;
}

ZephyrCosim *ZephyrCosim::m_inst = 0;



// TODO: Need functions from 'posix_soc_if.h'

// TODO: Need implementation of functions from 'zephyr_cosim_sysrw .h'

// TODO: need a 'main' function
