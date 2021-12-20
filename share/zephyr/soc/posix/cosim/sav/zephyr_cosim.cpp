/*
 * zephyr_cosim.cpp
 *
 *  Created on: Sep 27, 2021
 *      Author: mballance
 */
#include <stdio.h>
#include "zephyr_cosim.h"
#include "zephyr_cosim_sysrw.h"
#include "tblink_rpc/tblink_rpc.h"
#include "tblink_rpc/loader.h"
#include "EndpointServicesZephyrCosim.h"
#include "ZephyrCosimIf.h"

using namespace tblink_rpc_core;

static ZephyrCosimIf *prv_cosim_if = 0;
static IEndpoint *prv_endpoint = 0;

uint8_t sys_read8(mem_addr_t addr) {
	fprintf(stdout, "sys_read8\n");
	fflush(stdout);
}

void sys_write8(uint8_t data, mem_addr_t addr) {
	fprintf(stdout, "sys_write8\n");
	fflush(stdout);
	;
}

uint16_t sys_read16(mem_addr_t addr) {
	fprintf(stdout, "sys_read16\n");
	fflush(stdout);
	;
}

void sys_write16(uint16_t data, mem_addr_t addr) {
	fprintf(stdout, "sys_write16\n");
	fflush(stdout);
	;
}

uint32_t sys_read32(mem_addr_t addr) {
	fprintf(stdout, "sys_read32\n");
	fflush(stdout);
	uint32_t val = prv_cosim_if->read32(addr);
	fprintf(stdout, "sys_read32: val=0x%08x\n", val);
	fflush(stdout);

	return val;
}

void sys_write32(uint32_t data, mem_addr_t addr) {
	fprintf(stdout, "sys_write32\n");
	fflush(stdout);
	prv_cosim_if->write32(data, addr);
}

void zephyr_cosim_exit(int exit_code) {
	fprintf(stdout, "zephyr_cosim_exit: %d\n", exit_code);
	fflush(stdout);

	if (prv_endpoint) {
		// Notify that we're exiting
		prv_endpoint->shutdown();
	}
}

int zephyr_cosim_init(int argc, char **argv) {
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
	prv_endpoint = result.first;
	if (prv_endpoint->init(services, 0) == -1) {
		fprintf(stdout, "Initialization failed: %s\n", result.first->last_error().c_str());
		return -1;
	}

	int rv;

	prv_cosim_if = new ZephyrCosimIf(prv_endpoint);

	while ((rv=prv_endpoint->is_init()) == 0) {
		if (prv_endpoint->process_one_message() == -1) {
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
	if (prv_endpoint->build_complete() == -1) {
		fprintf(stdout, "Error: build phase failed: %s\n", prv_endpoint->last_error().c_str());
		return -1;
	}

	while ((rv=prv_endpoint->is_build_complete()) == 0) {
		if (prv_endpoint->process_one_message() == -1) {
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
	if (prv_endpoint->connect_complete() == -1) {
		fprintf(stdout, "Error: connect phase failed: %s\n", prv_endpoint->last_error().c_str());
		return -1;
	}

	while ((rv=prv_endpoint->is_connect_complete()) == 0) {
		if (prv_endpoint->process_one_message() == -1) {
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

int zephyr_cosim_process() {
	return prv_endpoint->process_one_message();
}

int zephyr_cosim_release() {
	prv_endpoint->update_comm_mode(IEndpoint::Automatic, IEndpoint::Released);
	return 0;
}

