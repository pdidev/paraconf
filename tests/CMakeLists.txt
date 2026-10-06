# Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
#               root of the project or at https://github.com/pdidev/paraconf
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.22...4.2)

# Only the tests are written in C++, the library itself requires no C++ compiler
enable_language(CXX)

if(NOT TARGET GTest::gtest)
	option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)
	add_subdirectory("../vendor/googletest-56efe39/" "googletest" EXCLUDE_FROM_ALL)
endif()
include(GoogleTest)

add_executable(paraconf_unit_tests
	api.cxx
	get.cxx
	location.cxx
	parse.cxx
	scalars.cxx
	status.cxx
	version.cxx
)
target_link_libraries(paraconf_unit_tests paraconf::paraconf GTest::gmock GTest::gtest GTest::gtest_main Threads::Threads)
gtest_discover_tests(paraconf_unit_tests)

# paraconf compiled again with its allocations redirected to pc_test_malloc, that the tests make fail on demand
get_target_property(paraconf_sources paraconf SOURCES)
list(TRANSFORM paraconf_sources PREPEND "${paraconf_SOURCE_DIR}/")
add_library(paraconf_failing_alloc STATIC ${paraconf_sources})
target_compile_definitions(paraconf_failing_alloc PRIVATE malloc=pc_test_malloc)
target_include_directories(paraconf_failing_alloc PUBLIC "$<TARGET_PROPERTY:paraconf,INCLUDE_DIRECTORIES>")
target_link_libraries(paraconf_failing_alloc PUBLIC Threads::Threads yaml)
set_property(TARGET paraconf_failing_alloc PROPERTY C_STANDARD 99)
set_property(TARGET paraconf_failing_alloc PROPERTY C_STANDARD_REQUIRED TRUE)

add_executable(paraconf_alloc_failure_tests alloc_failures.cxx)
target_link_libraries(paraconf_alloc_failure_tests paraconf_failing_alloc GTest::gmock GTest::gtest GTest::gtest_main Threads::Threads)
gtest_discover_tests(paraconf_alloc_failure_tests)

if("${BUILD_FORTRAN}")
	add_executable(fortran_test fortran_test.f90)
	target_link_libraries(fortran_test paraconf::paraconf_f90)
	add_test(NAME fortran_test COMMAND fortran_test "${CMAKE_CURRENT_SOURCE_DIR}/fortran_data.yml")
endif()
