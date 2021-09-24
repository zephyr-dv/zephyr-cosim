MKDV_MK := $(abspath $(lastword $(MAKEFILE_LIST)))
TEST_DIR := $(dir $(MKDV_MK))
MKDV_TOOL ?= questa
SOC_ROOT:=$(shell PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m zephyr_cosim soc-root)
TBLINK_UVM_FILES:=$(shell PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m tblink_rpc_hdl files sv-uvm)
TBLINK_PLUGIN:=$(shell PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m tblink_rpc_hdl simplugin)
ZEPHYR_COSIM_SV_FILES:=$(shell PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m zephyr_cosim sv-files)

MKDV_BUILD_DEPS += $(MKDV_CACHEDIR)/zephyr
MKDV_BUILD_DEPS += $(MKDV_CACHEDIR)/sw_tests

MKDV_VL_SRCS += $(TBLINK_UVM_FILES)
MKDV_VL_SRCS += $(ZEPHYR_COSIM_SV_FILES)
MKDV_VL_SRCS += $(TEST_DIR)/sv_smoke_tb.sv
TOP_MODULE = sv_smoke_tb

DPI_LIBS += $(TBLINK_PLUGIN)

ZEPHYR_BASE=$(PACKAGES_DIR)/zephyr
export ZEPHYR_BASE

# Specify path to the software to launch
MKDV_RUN_ARGS += +zephyr-cosim-exe=$(MKDV_CACHEDIR)/sw_tests/zephyr/zephyr.exe

include $(TEST_DIR)/../common/defs_rules.mk
RULES := 1

include $(TEST_DIR)/../common/defs_rules.mk

#********************************************************************
#* Create board structure
#********************************************************************
$(MKDV_CACHEDIR)/zephyr : 
	$(Q)rm -rf $@
	$(Q)PATH=$(PACKAGES_DIR)/python/bin:$(PATH) python3 -m zephyr_cosim \
		gen-board sv_smoke -o $@/boards/posix/sv_smoke

#********************************************************************
#* Build test software
#********************************************************************
$(MKDV_CACHEDIR)/sw_tests : $(MKDV_CACHEDIR)/zephyr
	$(Q)mkdir -p $(MKDV_CACHEDIR)/sw_tests
	$(Q)cd $(MKDV_CACHEDIR)/sw_tests ; \
		PACKAGES_DIR=$(PACKAGES_DIR) cmake $(TEST_DIR)/sw_tests \
		-DZEPHYR_BASE=$(PACKAGES_DIR)/zephyr \
		-DSOC_ROOT=$(SOC_ROOT) \
		-DBOARD_ROOT=$(MKDV_CACHEDIR)/zephyr \
		-DBOARD=sv_smoke_64
	$(Q) PATH=$(PACKAGES_DIR)/python/bin:$(PATH) \
		$(MAKE) -j`nproc` -C $(MKDV_CACHEDIR)/sw_tests || rm -rf $@
	


