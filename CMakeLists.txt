cmake_minimum_required(VERSION 3.0)
project(paraconf LANGUAGES C)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(MPI REQUIRED)
find_package(LibYaml REQUIRED)

file(READ VERSION Paraconf_VERSION)

add_library(paraconf
	src/api.c
)
target_link_libraries(paraconf mpi yaml)
target_include_directories(paraconf
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/>
)

configure_file(cmake/paraconfConfigVersion.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/paraconfConfigVersion.cmake" @ONLY)

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
