# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sw_tests)

set_property(TARGET compiler-cpp PROPERTY no_exceptions)

# Disable rtti in C++
set_property(TARGET compiler-cpp PROPERTY no_rtti)


target_sources(app PRIVATE 
	$ENV{PACKAGES_DIR}/googletest/googletest/src/gtest-all.cc
	main.c
	)

target_link_libraries(app PRIVATE
	stdc++
	)

target_include_directories(app PRIVATE
	$ENV{PACKAGES_DIR}/googletest/googletest
	$ENV{PACKAGES_DIR}/googletest/googletest/include
	)
	
	
