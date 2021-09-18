ZEPHYR_COSIM_VE_COMMON_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ZEPHYR_COSIM_DIR := $(abspath $(ZEPHYR_COSIM_VE_COMMON_DIR)/../..)
PACKAGES_DIR := $(ZEPHYR_COSIM_DIR)/packages
DV_MK := $(shell PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m mkdv mkfile)

ifneq (1,$(RULES))

MKDV_PYTHONPATH += $(ZEPHYR_COSIM_DIR)/src

include $(DV_MK)
else # Rules
include $(DV_MK)

endif


