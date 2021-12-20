/*
 * zephyr_cosim_sysrw.cpp
 *
 *  Created on: Dec 19, 2021
 *      Author: mballance
 */

#include "zephyr_cosim_sysrw.h"
#include "ZephyrCosim.h"

uint8_t sys_read8(mem_addr_t addr) {
	return ZephyrCosim::inst()->read8(addr);
}

void sys_write8(uint8_t data, mem_addr_t addr) {
	ZephyrCosim::inst()->write8(data, addr);
}

uint16_t sys_read16(mem_addr_t addr) {
	return ZephyrCosim::inst()->read16(addr);
}

void sys_write16(uint16_t data, mem_addr_t addr) {
	ZephyrCosim::inst()->write16(data, addr);
}

uint32_t sys_read32(mem_addr_t addr) {
	return ZephyrCosim::inst()->read32(addr);
}

void sys_write32(uint32_t data, mem_addr_t addr) {
	ZephyrCosim::inst()->write32(data, addr);
}

