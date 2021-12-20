/*
 * zephyr_cosim_main.cpp
 *
 *  Created on: Dec 20, 2021
 *      Author: mballance
 */

#include "ZephyrCosim.h"

int main(int argc, char **argv) {
	ZephyrCosim *zc = ZephyrCosim::inst();

	return zc->run(argc, argv);
}


