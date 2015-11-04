cmake_minimum_required(VERSION 3.0)
project (yaml LANGUAGES C)

option(BUILD_SHARED_LIBS 
	"Build libyaml as a shared library instead of a static one"
	ON
)
set (YAML_VERSION_MAJOR 0)
set (YAML_VERSION_MINOR 1)
set (YAML_VERSION_PATCH 6)
set (YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")

file (GLOB SRC src/*.c)

add_library(yaml
	${SRC}
)
target_include_directories(yaml
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/win32/>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/>
)

add_definitions (-DHAVE_CONFIG_H -DYAML_DECLARE_STATIC)

# configure_file(cmake/yamlConfigVersion.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/yamlConfigVersion.cmake" @ONLY)
# 
install(TARGETS yaml
	EXPORT yaml
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
	INCLUDES DESTINATION include
)
install(FILES include/yaml.h
	DESTINATION include
)
install(EXPORT yaml DESTINATION "lib/CMake/yaml" )
install(FILES
		"cmake/yamlConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/yamlConfigVersion.cmake"
	DESTINATION "lib/CMake/yaml"
)
