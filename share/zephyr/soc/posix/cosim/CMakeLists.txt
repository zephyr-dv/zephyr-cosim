# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_compile_definitions(NO_POSIX_CHEATS)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tblink_rpc_core)
set(TBLINK_RPC_C_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tblink_rpc_core)
set(JSON_RPC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tblink_rpc_core)
message("local tblink_rpc exists")
else()
message("local tblink_rpc does not exist")
set(TBLINK_RPC_C_DIR $ENV{PACKAGES_DIR}/tblink-rpc-core/cpp)
set(JSON_RPC_DIR $ENV{PACKAGES_DIR}/json/include)
#file(GLOB_RECURSE TBLINK_RPC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../packages/tblink-rpc-core/c/*.c)
#file(GLOB_RECURSE TBLINK_RPC_SRCS /project/fun/zephyr-cosim/zephyr-cosim/packages/tblink-rpc-core/c/*.c)
message("  files ${TBLINK_RPC_SRCS}")
endif()

#file(GLOB_RECURSE TBLINK_RPC_SRCS ${TBLINK_RPC_C_DIR}/*.cpp)

zephyr_library_sources(
	irq_ctrl.c
	irq_handler.c
	soc.c
	tracing.c
	EndpointServicesZephyrCosim.cpp
	zephyr_cosim.cpp
#	${TBLINK_RPC_SRCS}
	)

# Forcing this include prevents static-inline implementations of
# sys_read/sys_write functions from being used, allowing us to
# hook them
#
# Note: the SHELL prefix prevents cmake from de-duplicating the
# '-include' option, which may have been provided elsewhere
zephyr_compile_options(
  "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/zephyr_cosim_sysrw.h"
  )


zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/posix/include
  ${TBLINK_RPC_C_DIR}/include
  ${JSON_RPC_DIR}
  )

message("DTS_ROOT_BINDINGS=${DTS_ROOT_BINDINGS}")
