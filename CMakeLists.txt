cmake_minimum_required(VERSION 3.0)
# enable C++ to work-around cmake issue #15829 https://cmake.org/Bug/view.php?id=15829
project(paraconf LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


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

file(READ VERSION Paraconf_VERSION)


# the actual library

add_library(paraconf
	src/api.c
	src/ytype.c
)
include(GenerateExportHeader)
generate_export_header(paraconf)
set_target_properties(paraconf PROPERTIES C_VISIBILITY_PRESET hidden)
target_include_directories(paraconf
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/>
)
target_link_libraries(paraconf mpi yaml)


# installation

configure_file(cmake/paraconfConfigVersion.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/paraconfConfigVersion.cmake"
	@ONLY
)
install(TARGETS paraconf
	EXPORT paraconf
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
	INCLUDES DESTINATION include
)
install(FILES include/paraconf.h
	DESTINATION include
)
install(EXPORT paraconf DESTINATION "lib/CMake/paraconf" )
install(FILES
		"cmake/FindMPI.cmake"
		"cmake/FindLibYaml.cmake"
		"cmake/paraconfConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/paraconfConfigVersion.cmake"
	DESTINATION "lib/CMake/paraconf"
)
