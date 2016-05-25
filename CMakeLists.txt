#=============================================================================
# Copyright 2015 CEA, Julien Bigot <julien.bigot@cea.fr>
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the names of CEA, nor the names of the contributors may be used to
#   endorse or promote products derived from this software without specific
#   prior written  permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

cmake_minimum_required(VERSION 3.0)
# enable C++ to work-around cmake issue #15829 https://cmake.org/Bug/view.php?id=15829
project(paraconf)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)


# Build options

option(BUILD_SHARED_LIBS 
	"Build paraconf as a shared library instead of a static one"
	ON
)
option(USE_SYSTEM_YAML
	"Use an already installed libyaml instead of building the embedded one"
	ON
)


# dependancies

find_package(MPI REQUIRED)
if("${USE_SYSTEM_YAML}")
	find_package(LibYaml REQUIRED)
else()
	add_subdirectory(vendor/libyaml/)
endif()


# the version file

file(READ VERSION paraconf_VERSION)


# the actual library

add_library(paraconf
	src/api.c
	src/status.c
	src/ypath.c
	src/paraconf.f90
)
include(AddCCompilerFlags)
add_compiler_flags(paraconf PRIVATE -std=c99 )
include(GenerateExportHeader)
generate_export_header(paraconf)
set_target_properties(paraconf PROPERTIES C_VISIBILITY_PRESET hidden)
target_include_directories(paraconf
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)
target_link_libraries(paraconf mpi yaml)
# set_property(TARGET paraconf PROPERTY VERSION ${paraconf_VERSION})
# set_property(TARGET paraconf PROPERTY SOVERSION 3)
# set_property(TARGET paraconf PROPERTY INTERFACE_paraconf_MAJOR_VERSION 3)
# set_property(TARGET paraconf APPEND PROPERTY COMPATIBLE_INTERFACE_STRING paraconf_MAJOR_VERSION)

# the test example


add_subdirectory(example/)



# library installation

install(TARGETS paraconf EXPORT paraconfTargets
	LIBRARY DESTINATION lib INCLUDES DESTINATION include
	ARCHIVE DESTINATION lib INCLUDES DESTINATION include
)
install(FILES
		include/paraconf.h
		"${CMAKE_CURRENT_BINARY_DIR}/paraconf_export.h"
	DESTINATION include
)


# cmake stuff installation

set(ConfigPackageLocation "lib/CMake/paraconf-${paraconf_VERSION}")
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/paraconf/paraconfConfigVersion.cmake"
	VERSION ${paraconf_VERSION}
	COMPATIBILITY AnyNewerVersion
)
install(EXPORT paraconfTargets DESTINATION ${ConfigPackageLocation})
install(FILES
		cmake/FindMPI.cmake
		cmake/FindLibYaml.cmake
		cmake/paraconfConfig.cmake
		"${CMAKE_CURRENT_BINARY_DIR}/paraconf/paraconfConfigVersion.cmake"
	DESTINATION ${ConfigPackageLocation}
)
