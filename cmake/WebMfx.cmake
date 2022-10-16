set(EMSDK $ENV{EMSDK} CACHE STRING "Path to the root of Emscripten SDK")

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


macro(add_emscripten_executable Target)
	set(options)
	set(oneValueArgs SOURCE_MAP_BASE)
	set(multiValueArgs SRC INCLUDE SETTINGS COMPILE_SETTINGS LINK_SETTINGS LINK_OPTIONS BINDINGS)
	cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	if (NOT DEFINED ARG_SOURCE_MAP_BASE)
		set(ARG_SOURCE_MAP_BASE http://127.0.0.1:8888/build/)
	endif()

	list(TRANSFORM ARG_SETTINGS PREPEND -s)
	list(TRANSFORM ARG_COMPILE_SETTINGS PREPEND -s)
	list(TRANSFORM ARG_LINK_SETTINGS PREPEND -s)

	# First add custom command targets for each binding
	set(AllBindingTargets "")
	foreach(Binding ${ARG_BINDINGS})
		set(idlSpecFile "${CMAKE_CURRENT_SOURCE_DIR}/${Binding}")
		cmake_path(GET idlSpecFile ROOT_DIRECTORY idlSpecRoot)
		cmake_path(GET idlSpecFile STEM idlSpecName LAST_ONLY)
		set(bindingDir "${CMAKE_CURRENT_BINARY_DIR}/${idlSpecRoot}/bindings/")
		set(bindingFilePrefix "${bindingDir}${idlSpecName}")

		add_custom_command(
			COMMAND ${EMSDK}/upstream/emscripten/tools/webidl_binder ${idlSpecFile} ${bindingFilePrefix}
			DEPENDS ${idlSpecFile}
			OUTPUT ${bindingFilePrefix}.cpp ${bindingFilePrefix}.js
			COMMENT "Generating binding code from ${idlSpecFile}."
		)

		set(BindingTarget "${Target}_BINDINGS_${idlSpecName}")
		add_custom_target(
			${BindingTarget}
			DEPENDS
				${bindingFilePrefix}.cpp
				${bindingFilePrefix}.js
		)

		list(APPEND ARG_LINK_OPTIONS --post-js ${bindingFilePrefix}.js)
		list(APPEND ARG_INCLUDE ${bindingDir})
		list(APPEND AllBindingTargets ${BindingTarget})
	endforeach()

	# Then add the main target
	add_executable(${Target} ${ARG_SRC})

	target_compile_options(
		${Target}
		PRIVATE
			${ARG_SETTINGS}
			${ARG_COMPILE_SETTINGS}
	)

	set(DEBUG_LINK_OPTIONS
		-g -gsource-map
		--source-map-base ${ARG_SOURCE_MAP_BASE}
		-Wno-limited-postlink-optimizations
	)

	target_link_options(
		${Target}
		PRIVATE
			${ARG_SETTINGS}
			${ARG_LINK_SETTINGS}
			${ARG_LINK_OPTIONS}
			$<$<CONFIG:Debug>:${DEBUG_LINK_OPTIONS}>
	)

	target_include_directories(
		${Target}
		PRIVATE
			${ARG_INCLUDE}
	)

	foreach(BindingTarget ${AllBindingTargets})
		add_dependencies(
			${Target}
			${BindingTarget}
		)
	endforeach()
endmacro()

