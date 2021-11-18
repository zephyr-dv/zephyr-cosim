/*
 * ZephyrCosimIf.h
 *
 *  Created on: Sep 28, 2021
 *      Author: mballance
 */

#pragma once
#include "tblink_rpc/IEndpoint.h"
#include "tblink_rpc/IInterfaceInst.h"
#include "tblink_rpc/IInterfaceType.h"

class ZephyrCosimIf {
public:
	ZephyrCosimIf(
			tblink_rpc_core::IEndpoint		*ep,
			const std::string 				&name="cosim");

	virtual ~ZephyrCosimIf();

	void write8(uint8_t data, uint64_t addr);

	uint8_t read8(uint64_t addr);

	void write16(uint16_t data, uint64_t addr);

	uint16_t read16(uint64_t addr);

	void write32(uint32_t data, uint64_t addr);

	uint32_t read32(uint64_t addr);

	void sys_irq(uint8_t num);

	static tblink_rpc_core::IInterfaceType *registerType(
			tblink_rpc_core::IEndpoint *ep);

private:
	void request_f(
			tblink_rpc_core::IInterfaceInst		*ifinst,
			tblink_rpc_core::IMethodType		*method,
			intptr_t							call_id,
			tblink_rpc_core::IParamValVec		*params);

private:
	tblink_rpc_core::IInterfaceInst				*m_ifinst;
	tblink_rpc_core::IMethodType				*m_write8;
	tblink_rpc_core::IMethodType				*m_read8;
	tblink_rpc_core::IMethodType				*m_write16;
	tblink_rpc_core::IMethodType				*m_read16;
	tblink_rpc_core::IMethodType				*m_write32;
	tblink_rpc_core::IMethodType				*m_read32;
	tblink_rpc_core::IMethodType				*m_sys_irq;

};

