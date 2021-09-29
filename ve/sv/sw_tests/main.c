/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/sys_io.h>

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);
	sys_write32(0, 24);
	sys_write32(0, 36);
	sys_write32(0, 48);
}
