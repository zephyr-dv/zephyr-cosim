/*
 * zephyr_cosim.h
 *
 *  Created on: Sep 27, 2021
 *      Author: mballance
 */

#ifndef INCLUDED_ZEPHYR_COSIM_H
#define INCLUDED_ZEPHYR_COSIM_H

#ifdef __cplusplus
extern "C" {
#endif

int zephyr_cosim_init(int argc, char **argv);

int zephyr_cosim_process();

int zephyr_cosim_release();

void zephyr_cosim_exit(int exit_code);


#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_ZEPHYR_COSIM_H */
