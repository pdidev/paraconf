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

cmake_minimum_required(VERSION 3.5)
# enable C++ to work-around cmake issue #15829 https://cmake.org/Bug/view.php?id=15829
project(paraconf LANGUAGES C CXX Fortran)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(POLICY CMP0063)
cmake_policy(SET CMP0063 NEW)
endif()

include(AddCCompilerFlags)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ExternalProject)


# Build options

option(BUILD_SHARED_LIBS 
	"Build paraconf as a shared library instead of a static one"
	ON
)
option(ENABLE_FORTRAN
	"Enable compilation of the Fortran version of the library"
	ON
)
option(USE_SYSTEM_YAML
	"Use an already installed libyaml instead of building the embedded one"
	ON
)

get_property(ENABLED_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
if("${ENABLE_FORTRAN}" AND NOT "${ENABLED_LANGUAGES}" MATCHES Fortran)
	message(FATAL_ERROR "Fortran language support disabled can not build Fortran lib, set ENABLE_FORTRAN to OFF")
endif()


# Find dependancies

find_package(MPI REQUIRED)
find_package(Threads REQUIRED)
if("${USE_SYSTEM_YAML}")
	find_package(LibYaml REQUIRED)
else()
	if("${BUILD_SHARED_LIBS}" OR "${CMAKE_POSITION_INDEPENDENT_CODE}")
		set(YAML_PIC "--with-pic")
	else()
		set(YAML_PIC "--without-pic")
	endif()
	ExternalProject_Add(LibYaml
			EXCLUDE_FROM_ALL 1
			URL "${CMAKE_CURRENT_SOURCE_DIR}/vendor/yaml-0.1.7.tar.gz"
			URL_HASH SHA512=1ee5007dd10db137b5ee80f8117f07390ec04af98d087a5f5475dd2b38d87c699b79ab1676e6c7bfa263323fcdf8edd69fada2b0b7f9c57bef4e46cd65f1e975
			SOURCE_DIR LibYaml-src/
			BINARY_DIR LibYaml-build/
			INSTALL_DIR LibYaml-install/
			CONFIGURE_COMMAND "<SOURCE_DIR>/configure"
					"--enable-static"
					"${YAML_PIC}"
					"--prefix=<INSTALL_DIR>/"
					"CC=${CMAKE_C_COMPILER}"
					"CFLAGS=${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}}"
					"LDFLAGS=${CMAKE_STATIC_LINKER_FLAGS} ${CMAKE_STATIC_LINKER_FLAGS_${CMAKE_BUILD_TYPE}}"
			INSTALL_COMMAND make install
	)
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/LibYaml-install/include")
	add_library(yaml STATIC IMPORTED)
	add_dependencies(yaml LibYaml)
	set_target_properties(yaml PROPERTIES
			IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/LibYaml-install/lib/libyaml.a"
			INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/LibYaml-install/include")
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/LibYaml-install/include/"
		DESTINATION include
	)
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/LibYaml-install/lib/" 
		DESTINATION lib
	)
endif()


# Version

file(READ VERSION paraconf_VERSION)
string(STRIP "${paraconf_VERSION}" paraconf_VERSION)
string(REPLACE "." ";" paraconf_VERSION_list ${paraconf_VERSION})
list(GET paraconf_VERSION_list 0 paraconf_VERSION_MAJOR)
list(GET paraconf_VERSION_list 1 paraconf_VERSION_MINOR)
list(GET paraconf_VERSION_list 2 paraconf_VERSION_PATCH)


# Libraries

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#C version
add_library(paraconf
	src/api.c
	src/status.c
	src/ypath.c
)
add_compiler_flags(paraconf PRIVATE -std=c99 -Wall -Wextra)
generate_export_header(paraconf)
set_property(TARGET paraconf PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET paraconf PROPERTY VERSION ${paraconf_VERSION})
set_property(TARGET paraconf PROPERTY SOVERSION ${paraconf_VERSION_MAJOR})
set_property(TARGET paraconf PROPERTY paraconf_MAJOR_VERSION ${paraconf_VERSION_MAJOR})
set_property(TARGET paraconf APPEND PROPERTY COMPATIBLE_INTERFACE_STRING paraconf_MAJOR_VERSION)
target_include_directories(paraconf PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>"
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>"
	"$<INSTALL_INTERFACE:include>"
)
target_link_libraries(paraconf Threads::Threads MPI::MPI_C yaml)
install(TARGETS paraconf EXPORT PC_export
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(FILES include/paraconf.h "${CMAKE_CURRENT_BINARY_DIR}/paraconf_export.h"
	DESTINATION include
)

#Fortran version
if("${ENABLE_FORTRAN}")
	add_library(paraconf_f90 src/paraconf.F90)
	target_link_libraries(paraconf_f90 paraconf MPI::MPI_Fortran)
	set_property(TARGET paraconf_f90 PROPERTY Fortran_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/modinclude")
	target_include_directories(paraconf_f90 PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/modinclude/>"
		"$<INSTALL_INTERFACE:lib/paraconf-${paraconf_VERSION}/finclude>")
	set_property(TARGET paraconf_f90 PROPERTY VERSION ${paraconf_VERSION})
	set_property(TARGET paraconf_f90 PROPERTY SOVERSION ${paraconf_VERSION_MAJOR})
	set_property(TARGET paraconf_f90 PROPERTY paraconf_MAJOR_VERSION ${paraconf_VERSION_MAJOR})
	set_property(TARGET paraconf_f90 APPEND PROPERTY COMPATIBLE_INTERFACE_STRING paraconf_MAJOR_VERSION)
	install(TARGETS paraconf_f90 EXPORT PC_export
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/modinclude/
		DESTINATION "lib/paraconf-${paraconf_VERSION}/finclude"
	)
endif()


# Test examples

enable_testing()
add_subdirectory(example/)


# Installable config

set(ConfigPackageLocation "share/paraconf-${paraconf_VERSION}/cmake/")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/paraconf/paraconfConfigVersion.cmake"
	VERSION ${paraconf_VERSION}
	COMPATIBILITY AnyNewerVersion
)
install(EXPORT PC_export DESTINATION "${ConfigPackageLocation}" FILE paraconf.cmake)
install(FILES
		cmake/FindLibYaml.cmake
		cmake/FindMPI.cmake
		cmake/FindPackageHandleStandardArgs.cmake
		cmake/FindPackageMessage.cmake
		cmake/paraconfConfig.cmake
		"${CMAKE_CURRENT_BINARY_DIR}/paraconf/paraconfConfigVersion.cmake"
	DESTINATION "${ConfigPackageLocation}"
)
install(DIRECTORY cmake/FindMPI DESTINATION "${ConfigPackageLocation}")
