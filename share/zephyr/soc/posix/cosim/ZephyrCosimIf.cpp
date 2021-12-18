/*
 * ZephyrCosimIf.cpp
 *
 *  Created on: Sep 28, 2021
 *      Author: mballance
 */

#include "ZephyrCosimIf.h"
#include "tblink_rpc/IInterfaceTypeBuilder.h"

using namespace tblink_rpc_core;

ZephyrCosimIf::ZephyrCosimIf(
		IEndpoint			*ep,
		const std::string	&name) {
	m_ep = ep;
	IInterfaceType *iftype = registerType(ep);

	m_ifinst = ep->defineInterfaceInst(
			iftype,
			name,
			true,
			std::bind(&ZephyrCosimIf::request_f,
					this,
					std::placeholders::_1,
					std::placeholders::_2,
					std::placeholders::_3,
					std::placeholders::_4));
	m_write8  = iftype->findMethod("sys_write8");
	m_read8   = iftype->findMethod("sys_read8");
	m_write16 = iftype->findMethod("sys_write16");
	m_read16  = iftype->findMethod("sys_read16");
	m_write32 = iftype->findMethod("sys_write32");
	m_read32  = iftype->findMethod("sys_read32");
	m_sys_irq = iftype->findMethod("sys_irq");
}

ZephyrCosimIf::~ZephyrCosimIf() {
	// TODO Auto-generated destructor stub
}

void ZephyrCosimIf::write8(uint8_t data, uint64_t addr) {
	;
}

uint8_t ZephyrCosimIf::read8(uint64_t addr) {
	;
}

void ZephyrCosimIf::write16(uint16_t data, uint64_t addr) {
	;
}

uint16_t ZephyrCosimIf::read16(uint64_t addr) {
	;
}

void ZephyrCosimIf::write32(uint32_t data, uint64_t addr) {
	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(data, 32));
	params->push_back(m_ifinst->mkValIntU(addr, 64));

	fprintf(stdout, "--> invoke::write32\n");
	fflush(stdout);

	bool complete = false;

	m_ifinst->invoke_nb(
			m_write32,
			params,
			[&](IParamVal *retval) {
				complete = true;
			});
	fprintf(stdout, "<-- invoke::write32\n");
	fflush(stdout);

	while (!complete) {
		fprintf(stdout, "--> process_one_message\n");
		fflush(stdout);
		int rv = m_ep->process_one_message();
		fprintf(stdout, "<-- process_one_message %d\n", rv);
		fflush(stdout);

		if (rv == -1) {
			break;
		}
	}
}

uint32_t ZephyrCosimIf::read32(uint64_t addr) {
	IParamValVec *params = m_ifinst->mkValVec();
	params->push_back(m_ifinst->mkValIntU(addr, 64));


	fprintf(stdout, "--> invoke::read32\n");
	fflush(stdout);
	bool complete = false;
	IParamVal *ret = 0;

	m_ifinst->invoke_nb(
			m_read32,
			params,
			[&](IParamVal *retval) {
				complete = true;
				ret = retval->clone();
			});

	while (!complete) {
		int rv = m_ep->process_one_message();

		if (rv == -1) {
			break;
		}
	}

	fprintf(stdout, "<-- invoke::read32 %p\n", ret);
	fflush(stdout);

	uint32_t rval = dynamic_cast<IParamValInt *>(ret)->val_u();
	delete ret;

	return rval;
}


tblink_rpc_core::IInterfaceType *ZephyrCosimIf::registerType(
			tblink_rpc_core::IEndpoint *ep) {
	IInterfaceType *iftype = ep->findInterfaceType("zephyr_cosim_if");

	if (!iftype) {
		IInterfaceTypeBuilder *iftype_b = ep->newInterfaceTypeBuilder(
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
		iftype_b->add_method(method_b);

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
		iftype_b->add_method(method_b);


		// read16
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_read16",
				3,
				iftype_b->mkTypeInt(false, 16),
				true,
				true);
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		iftype_b->add_method(method_b);

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
		iftype_b->add_method(method_b);

		// read32
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_read32",
				5,
				iftype_b->mkTypeInt(false, 32),
				true,
				true);
		method_b->add_param("addr",
				iftype_b->mkTypeInt(false, 64));
		iftype_b->add_method(method_b);

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
		iftype_b->add_method(method_b);

		// irq
		method_b = iftype_b->newMethodTypeBuilder(
				"sys_irq",
				7,
				0,
				false,
				false);
		method_b->add_param("num",
				iftype_b->mkTypeInt(false, 8));
		iftype_b->add_method(method_b);

		iftype = ep->defineInterfaceType(iftype_b);
	}

	return iftype;
}

extern "C" void posix_interrupt_raised();
extern "C" void hw_irq_ctrl_raise_im(unsigned int irq);
extern "C" void hw_irq_ctrl_raise_im_from_sw(unsigned int irq);
extern "C" void hw_irq_ctrl_set_irq(unsigned int irq);

void ZephyrCosimIf::request_f(
		tblink_rpc_core::IInterfaceInst		*ifinst,
		tblink_rpc_core::IMethodType		*method,
		intptr_t							call_id,
		tblink_rpc_core::IParamValVec		*params) {
	if (method == m_sys_irq) {
		// TODO:
		fprintf(stdout, "TODO: IRQ\n");
		fflush(stdout);
		ifinst->invoke_rsp(call_id, 0);

//		posix_interrupt_raised();
//		hw_irq_ctrl_raise_im_from_sw(0);
//		hw_irq_ctrl_set_irq(0);
		hw_irq_ctrl_raise_im(0);


		fprintf(stdout, "--> Update run mode\n");
		fflush(stdout);
		m_ep->update_comm_mode(IEndpoint::Automatic, IEndpoint::Released);
		fprintf(stdout, "<-- Update run mode\n");
		fflush(stdout);
	}
}
