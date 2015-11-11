# - Finds Libyaml
#
# === Variables ===
#
# This module will set the following variables in your project:
#   LibYaml_FOUND           TRUE if FindLibYaml found LibYaml
#
# === Usage ===
#
# To use this module, simply run find_package(LibYaml) from a CMakeLists.txt.
# If you are happy with the auto-detected configuration, then you're done.
# If not, set both LibYaml_<lang>_LIBRARIES and  LibYaml_<lang>_INCLUDE_PATH.
# You may also set any other variables listed above, but these two are
# required. This will circumvent autodetection entirely.

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

# Protect against multiple inclusion, which would fail when already imported targets are added once more.
if(NOT TARGET yaml)

# include this to handle the QUIETLY and REQUIRED arguments

include(FindPackageHandleStandardArgs)
include(GetPrerequisites)
find_package(PkgConfig)

pkg_check_modules(LibYaml QUIET yaml-0.1)


# Create imported target

list(GET LibYaml_LIBRARIES 0 _LibYaml_LIBRARY_NAME)
set(_LibYaml_LIBRARIES_OTHER "${LibYaml_LIBRARIES}")
if(NOT "${_LibYaml_LIBRARIES_OTHER}" MATCHES "^ *$")
	list(REMOVE_AT _LibYaml_LIBRARIES_OTHER 0)
endif()
find_library(_LibYaml_LIBRARY "${_LibYaml_LIBRARY_NAME}" HINTS ${_LibYaml_LIBRARY_DIRS})
list(APPEND LibYaml_CFLAGS ${LibYaml_CFLAGS_OTHER})
list(APPEND LibYaml_LDFLAGS ${LibYaml_LDFLAGS_OTHER})
configure_file(${CMAKE_CURRENT_LIST_DIR}/LibYamlConfig.cmake.in LibYamlConfig.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/LibYamlConfig.cmake)
set(LibYaml_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/LibYamlConfig.cmake")

find_package_handle_standard_args(LibYaml DEFAULT_MSG LibYaml_LIBRARIES)

unset(_LibYaml_LIBRARIES_OTHER)
unset(_LibYaml_LIBRARY)
unset(_LibYaml_LIBRARY_NAME)

endif(NOT TARGET yaml) 
