/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * For all purposes, Zephyr threads see a CPU running at an infinitely high
 * clock.
 *
 * Therefore, the code will always run until completion after each interrupt,
 * after which arch_cpu_idle() will be called releasing the execution back to
 * the HW models.
 *
 * The HW models raising an interrupt will "awake the cpu" by calling
 * posix_interrupt_raised() which will transfer control to the irq handler,
 * which will run inside SW/Zephyr context. After which a arch_swap() to
 * whatever Zephyr thread may follow.  Again, once Zephyr is done, control is
 * given back to the HW models.
 *
 * The Zephyr OS+APP code and the HW models are gated by a mutex +
 * condition as there is no reason to let the zephyr threads run while the
 * HW models run or vice versa
 *
 */

#include "soc.h"

#include <zephyr.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// #include <sys/sys_io.h>
#include <arch/posix/posix_soc_if.h>

#include "posix_board_if.h"
#include "posix_soc.h"
#include "posix_core.h"
#include "posix_arch_internal.h"
#include "kernel_internal.h"
#include <devicetree.h>
#include "zephyr_cosim.h"

#define POSIX_ARCH_SOC_DEBUG_PRINTS 0

#define PREFIX "POSIX SOC: "
#define ERPREFIX PREFIX"error on "

#if POSIX_ARCH_SOC_DEBUG_PRINTS
#define PS_DEBUG(fmt, ...) posix_print_trace(PREFIX fmt, __VA_ARGS__)
#else
#define PS_DEBUG(...)
#endif

/* Conditional variable to know if the CPU is running or halted/idling */
static pthread_cond_t  cond_cpu  = PTHREAD_COND_INITIALIZER;
/* Mutex for the conditional variable posix_soc_cond_cpu */
static pthread_mutex_t mtx_cpu   = PTHREAD_MUTEX_INITIALIZER;
/* Variable which tells if the CPU is halted (1) or not (0) */
static bool cpu_halted = true;

static bool soc_terminate = false; /* Is the program being closed */


int posix_is_cpu_running(void)
{
	return !cpu_halted;
}


/**
 * Helper function which changes the status of the CPU (halted or running)
 * and waits until somebody else changes it to the opposite
 *
 * Both HW and SW threads will use this function to transfer control to the
 * other side.
 *
 * This is how the idle thread halts the CPU and gets halted until the HW models
 * raise a new interrupt; and how the HW models awake the CPU, and wait for it
 * to complete and go to idle.
 */
void posix_change_cpu_state_and_wait(bool halted)
{
	fprintf(stdout, "--> posix_change_cpu_state_and_wait: halted=%d\n", halted);
	fflush(stdout);

	PC_SAFE_CALL(pthread_mutex_lock(&mtx_cpu));

	PS_DEBUG("Going to halted = %d\n", halted);

	cpu_halted = halted;

	/* We let the other side know the CPU has changed state */
	fprintf(stdout, "--> call broadcast\n");
	fflush(stdout);
	PC_SAFE_CALL(pthread_cond_broadcast(&cond_cpu));
	fprintf(stdout, "<-- call broadcast\n");
	fflush(stdout);

	/* We wait until the CPU state has been changed. Either:
	 * we just awoke it, and therefore wait until the CPU has run until
	 * completion before continuing (before letting the HW models do
	 * anything else)
	 *  or
	 * we are just hanging it, and therefore wait until the HW models awake
	 * it again
	 */
	fprintf(stdout, "posix_change_cpu_state_and_wait: cpu_halted=%d halted=%d\n",
			cpu_halted, halted);
	while (cpu_halted == halted) {
		/* Here we unlock the mutex while waiting */
		fprintf(stdout, "--> cond_wait: cpu_halted=%d halted=%d\n",
				cpu_halted, halted);
		fflush(stdout);
		pthread_cond_wait(&cond_cpu, &mtx_cpu);
		fprintf(stdout, "<-- cond_wait: cpu_halted=%d halted=%d\n",
				cpu_halted, halted);
		fflush(stdout);
	}

	PS_DEBUG("Awaken after halted = %d\n", halted);

	PC_SAFE_CALL(pthread_mutex_unlock(&mtx_cpu));
	fprintf(stdout, "<-- posix_change_cpu_state_and_wait: halted=%d\n", halted);
	fflush(stdout);
}

/**
 * HW models shall call this function to "awake the CPU"
 * when they are raising an interrupt
 */
void posix_interrupt_raised(void)
{
	fprintf(stdout, "--> posix_interrupt_raised\n");
	fflush(stdout);
	/* We change the CPU to running state (we awake it), and block this
	 * thread until the CPU is hateld again
	 */
	posix_change_cpu_state_and_wait(false);

	/*
	 * If while the SW was running it was decided to terminate the execution
	 * we stop immediately.
	 */
	if (soc_terminate) {
		posix_exit(0);
	}

	fprintf(stdout, "<-- posix_interrupt_raised\n");
	fflush(stdout);
}








/**
 * Just a wrapper function to call Zephyr's z_cstart()
 * called from posix_boot_cpu()
 */
static void *zephyr_wrapper(void *a)
{

	return NULL;
}


/**
 * The HW models will call this function to "boot" the CPU
 * == spawn the Zephyr init thread, which will then spawn
 * anything it wants, and run until the CPU is set back to idle again
 */
void posix_boot_cpu(void)
{

}


/**
 * Clean up all memory allocated by the SOC and POSIX core
 *
 * This function can be called from both HW and SW threads
 */
void posix_soc_clean_up(void)
{
	/* LCOV_EXCL_START */ /* See Note1 */
	/*
	 * If we are being called from a HW thread we can cleanup
	 *
	 * Otherwise (!cpu_halted) we give back control to the HW thread and
	 * tell it to terminate ASAP
	 */
	if (cpu_halted) {

		posix_core_clean_up();
		run_native_tasks(_NATIVE_ON_EXIT_LEVEL);

	} else if (soc_terminate == false) {

		soc_terminate = true;

		PC_SAFE_CALL(pthread_mutex_lock(&mtx_cpu));

		cpu_halted = true;

		PC_SAFE_CALL(pthread_cond_broadcast(&cond_cpu));
		PC_SAFE_CALL(pthread_mutex_unlock(&mtx_cpu));

		while (1) {
			sleep(1);
			/* This SW thread will wait until being cancelled from
			 * the HW thread. sleep() is a cancellation point, so it
			 * won't really wait 1 second
			 */
		}
	}
	/* LCOV_EXCL_STOP */
}


#ifdef UNDEFINED
void sys_write8_impl(uint8_t data, mm_reg_t addr) {
	fprintf(stdout, "hook\n");
}

void sys_write32_impl(uint32_t data, mm_reg_t addr) {
	fprintf(stdout, "hook32\n");
}
#endif

int main(int argc, char **argv) {


}

/*
 * Notes about coverage:
 *
 * Note1: When the application is closed due to a SIGTERM, the path in this
 * function will depend on when that signal was received. Typically during a
 * regression run, both paths will be covered. But in some cases they won't.
 * Therefore and to avoid confusing developers with spurious coverage changes
 * we exclude this function from the coverage check
 *
 */
