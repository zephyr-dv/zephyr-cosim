/*
 * EndpointServicesZephyrCosim.h
 *
 *  Created on: Sep 27, 2021
 *      Author: mballance
 */

#pragma once
#include "tblink_rpc/IEndpointServices.h"


class EndpointServicesZephyrCosim : public tblink_rpc_core::IEndpointServices {
public:
	EndpointServicesZephyrCosim();

	virtual ~EndpointServicesZephyrCosim();

	virtual void init(tblink_rpc_core::IEndpoint *endpoint) override { };

	/**
	 * Return command-line arguments.
	 */
	virtual std::vector<std::string> args() override {
		return {};
	}

	virtual void shutdown() override { };

	virtual int32_t add_time_cb(
			uint64_t 		time,
			intptr_t		callback_id) override { return -1; };

	virtual void cancel_callback(intptr_t id) override { };

	virtual uint64_t time() override { return 0; };

	/**
	 * Returns the time precision as an exponent:
	 * 0: 1s
	 * -3: 1ms
	 * -6: 1us
	 * -9: 1ns
	 * -12: 1ps
	 */
	virtual int32_t time_precision() override { return -9; };

	// Release the environment to run
	virtual void run_until_event() override { };

	// Notify that we've hit an event
	virtual void hit_event() override { };

	virtual void idle() override { }
};

