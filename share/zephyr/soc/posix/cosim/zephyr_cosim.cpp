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

using namespace tblink_rpc_core;

uint8_t sys_read8(mem_addr_t addr) {
	fprintf(stdout, "sys_read8\n");
	;
}

void sys_write8(uint8_t data, mem_addr_t addr) {
	fprintf(stdout, "sys_write8\n");
	;
}

uint16_t sys_read16(mem_addr_t addr) {
	fprintf(stdout, "sys_read16\n");
	;
}

void sys_write16(uint16_t data, mem_addr_t addr) {
	fprintf(stdout, "sys_write16\n");
	;
}

uint32_t sys_read32(mem_addr_t addr) {
	fprintf(stdout, "sys_read32\n");
	;
}

void sys_write32(uint32_t data, mem_addr_t addr) {
	fprintf(stdout, "sys_write32\n");
	;
}

int zephyr_cosim_init(int argc, char **argv) {
	std::string tblink_rpc_lib;

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
	tblink_rpc_core::ILaunchParams *params = tblink->newLaunchParams();
	params->add_param("host", getenv("TBLINK_HOST"));
	params->add_param("port", getenv("TBLINK_PORT"));

	tblink_rpc_core::ILaunchType::result_t result = launch->launch(params);

	if (!result.first) {
		fprintf(stdout, "Failed: %s\n", result.second.c_str());
		return -1;
	} else {
		fprintf(stdout, "Succeeded\n");
	}

	EndpointServicesZephyrCosim *services = new EndpointServicesZephyrCosim();
	IEndpoint *ep = result.first;
	if (ep->init(services, 0) == -1) {
		fprintf(stdout, "Initialization failed: %s\n", result.first->last_error().c_str());
		return -1;
	}

	// TODO: register API type and inst

	if (ep->build_complete() == -1) {
		fprintf(stdout, "Error: build phase failed: %s\n", ep->last_error().c_str());
		return -1;
	}

	while (!ep->is_build_complete() == 0) {
		if (ep->process_one_message() == -1) {
			fprintf(stdout, "Error: failed waiting for build phase completion\n");
			return -1;
		}
	}

	if (ep->connect_complete() == -1) {
		fprintf(stdout, "Error: connect phase failed: %s\n", ep->last_error().c_str());
		return -1;
	}

	while (!ep->is_connect_complete() == 0) {
		if (ep->process_one_message() == -1) {
			fprintf(stdout, "Error: failed waiting for connect phase completion\n");
			return -1;
		}
	}




	return 0;
}

