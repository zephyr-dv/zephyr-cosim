MKDV_MK := $(abspath $(lastword $(MAKEFILE_LIST)))
TEST_DIR := $(dir $(MKDV_MK))
MKDV_TOOL ?= questa
SOC_ROOT:=$(shell PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m zephyr_cosim soc-root)

MKDV_BUILD_DEPS += $(MKDV_CACHEDIR)/zephyr
MKDV_BUILD_DEPS += $(MKDV_CACHEDIR)/sw_tests

MKDV_VL_SRCS += $(TEST_DIR)/sv_smoke_tb.sv
TOP_MODULE = sv_smoke_tb

ZEPHYR_BASE=$(PACKAGES_DIR)/zephyr
export ZEPHYR_BASE

include $(TEST_DIR)/../common/defs_rules.mk
RULES := 1

include $(TEST_DIR)/../common/defs_rules.mk

$(MKDV_CACHEDIR)/zephyr : 
	$(Q)rm -rf $@
	$(Q)PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m zephyr_cosim \
		gen-board sv_smoke -o $@/boards/posix/sv_smoke

$(MKDV_CACHEDIR)/sw_tests : $(MKDV_CACHEDIR)/zephyr
	$(Q)mkdir -p $(MKDV_CACHEDIR)/sw_tests
	$(Q)cd $(MKDV_CACHEDIR)/sw_tests ; \
		PACKAGES_DIR=$(PACKAGES_DIR) cmake $(TEST_DIR)/sw_tests \
		-DZEPHYR_BASE=$(PACKAGES_DIR)/zephyr \
		-DSOC_ROOT=$(SOC_ROOT) \
		-DBOARD_ROOT=$(MKDV_CACHEDIR)/zephyr \
		-DBOARD=sv_smoke_64
	$(Q) PATH=$(PACKAGES_DIR)/python/bin:$(PATH) \
		$(MAKE) -C $(MKDV_CACHEDIR)/sw_tests
	


