# Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
#               root of the project or at https://github.com/pdidev/paraconf
#
# SPDX-License-Identifier: MIT

include_guard()

# The build settings a user of paraconf sets are PARACONF_-prefixed, so that paraconf can be embedded with add_subdirectory() or
# FetchContent without its settings colliding with variables of the same name in the enclosing project.
#
# When paraconf is the top-level project, the historical unprefixed name still works and provides the default, which keeps existing
# command lines, scripts and packaging recipes working unchanged. When paraconf is embedded, the unprefixed name is ignored: it belongs to
# the enclosing project and says nothing about how paraconf should be built.

# Resolve the default of PARACONF_<NAME>, honouring an unprefixed <NAME> set by a top-level user.
# TRUTH says the setting is a boolean: ON/TRUE/1 and OFF/FALSE/0 then have to be compared as truth values rather than as text, or a
# preset saying `false' would look like it disagreed with `OFF'.
function(_paraconf_resolve_default TRUTH NAME DEFAULT OUTVAR)
	if("${PROJECT_IS_TOP_LEVEL}" AND DEFINED "${NAME}")
		if(NOT DEFINED "PARACONF_${NAME}")
			set(DEFAULT "${${NAME}}")
		else()
			if("${TRUTH}")
				set(_PARACONF_UNPREFIXED FALSE)
				set(_PARACONF_PREFIXED FALSE)
				if("${${NAME}}")
					set(_PARACONF_UNPREFIXED TRUE)
				endif()
				if("${PARACONF_${NAME}}")
					set(_PARACONF_PREFIXED TRUE)
				endif()
			else()
				set(_PARACONF_UNPREFIXED "${${NAME}}")
				set(_PARACONF_PREFIXED "${PARACONF_${NAME}}")
			endif()
			if(NOT "${_PARACONF_UNPREFIXED}" STREQUAL "${_PARACONF_PREFIXED}")
				# The fallback only applies while PARACONF_<NAME> is absent from the cache, so changing <NAME> on an already configured
				# build directory would otherwise be ignored silently.
				message(WARNING
					"Both ${NAME} and PARACONF_${NAME} are set and they disagree "
					"(${NAME}=${${NAME}}, PARACONF_${NAME}=${PARACONF_${NAME}}); PARACONF_${NAME} is the one that "
					"takes effect. Set PARACONF_${NAME}, or configure a fresh build directory.")
			endif()
		endif()
	endif()
	set("${OUTVAR}" "${DEFAULT}" PARENT_SCOPE)
endfunction()

### Declare a boolean build setting, exposed to the user as PARACONF_<NAME>
function(paraconf_option NAME DOCSTRING DEFAULT)
	_paraconf_resolve_default(TRUE "${NAME}" "${DEFAULT}" _PARACONF_DEFAULT)
	option("PARACONF_${NAME}" "${DOCSTRING}" "${_PARACONF_DEFAULT}")
endfunction()

### Declare a string-valued build setting, exposed to the user as PARACONF_<NAME>
function(paraconf_setting NAME DOCSTRING DEFAULT)
	_paraconf_resolve_default(FALSE "${NAME}" "${DEFAULT}" _PARACONF_DEFAULT)
	set("PARACONF_${NAME}" "${_PARACONF_DEFAULT}" CACHE STRING "${DOCSTRING}")
endfunction()
