macro(add_webmfx_library Target)
	set(options)
	set(oneValueArgs SOURCE_MAP_BASE)
	set(multiValueArgs SRC INCLUDE)
	cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	if (NOT DEFINED ARG_SOURCE_MAP_BASE)
		set(ARG_SOURCE_MAP_BASE http://127.0.0.1:8888/build/)
	endif()

	# Conceptually we want to do an add_library(${Target} SHARED)
	# but this is not supported by the emscripten toolchain so we fake it by
	# creating an executable but changing the extension and adding -sSIDE_MODULE
	add_executable(${Target} ${ARG_SRC})
	set_target_properties(${Target} PROPERTIES SUFFIX ".wasm")

	target_compile_options(${Target} PRIVATE
		-sSIDE_MODULE
	)

	set(DEBUG_LINK_OPTS
		-g -gsource-map
		--source-map-base ${ARG_SOURCE_MAP_BASE}
		-Wno-limited-postlink-optimizations
	)

	target_link_options(${Target} PRIVATE
	# This does not export any symbol for some unknown reason
	#	-sSIDE_MODULE=2
	#	-sEXPORTED_FUNCTIONS=OfxGetNumberOfPlugins,OfxGetPlugin
	# So we fall back on exporting all symbols...
		-sSIDE_MODULE=1
		$<$<CONFIG:Debug>:${DEBUG_LINK_OPTS}>
	)

	target_include_directories(${Target} PRIVATE ${ARG_INCLUDE})
endmacro()
