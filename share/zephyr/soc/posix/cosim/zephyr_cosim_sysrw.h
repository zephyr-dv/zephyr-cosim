/*
 * zephyr_cosim_sysrw.h
 *
 *  Created on: Sep 27, 2021
 *      Author: mballance
 */

#ifndef INCLUDED_ZEPHYR_COSIM_SYSRW_H
#define INCLUDED_ZEPHYR_COSIM_SYSRW_H
#include <toolchain.h>
#include <zephyr/types.h>
#include <sys/sys_io.h>

// Prevent the built-in sys_io.h content from being included
#define ZEPHYR_INCLUDE_ARCH_COMMON_SYS_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

uint8_t sys_read8(mem_addr_t addr);

void sys_write8(uint8_t data, mem_addr_t addr);

uint16_t sys_read16(mem_addr_t addr);

void sys_write16(uint16_t data, mem_addr_t addr);

uint32_t sys_read32(mem_addr_t addr);

void sys_write32(uint32_t data, mem_addr_t addr);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_ZEPHYR_COSIM_SYSRW_H */
