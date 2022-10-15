cmake_minimum_required(VERSION 3.24...3.24)

project(
	WebMfx
	VERSION 0.1
	LANGUAGES C CXX)

include(cmake/WebMfx.cmake)

add_webmfx_library(
	ExamplePlugin1
	SRC
		src/plugin.c
		src/openmfx-sdk/c/common/common.c
	INCLUDE
		src/openmfx
)

# Host

set(idlSpecFile "${CMAKE_CURRENT_SOURCE_DIR}/src/binding.idl")
cmake_path(GET idlSpecFile ROOT_DIRECTORY idlSpecRoot)
cmake_path(GET idlSpecFile STEM idlSpecName LAST_ONLY)
set(bindingDir "${CMAKE_CURRENT_BINARY_DIR}/${idlSpecRoot}/bindings/")
set(bindingFilePrefix "${bindingDir}${idlSpecName}")
add_custom_command(
    COMMAND $ENV{EMSDK}/upstream/emscripten/tools/webidl_binder ${idlSpecFile} ${bindingFilePrefix}
    DEPENDS ${idlSpecFile}
    OUTPUT ${bindingFilePrefix}.cpp ${bindingFilePrefix}.js
    COMMENT "Generating binding code from ${idlSpecFile}."
)

add_executable(WebMfxHost
	src/webmfx.cpp
	src/openmfx-sdk/c/common/common.c
	src/openmfx-sdk/c/host/types.c
	src/openmfx-sdk/c/host/meshEffectSuite.c
	src/openmfx-sdk/c/host/propertySuite.c
	src/openmfx-sdk/c/host/parameterSuite.c
	src/openmfx-sdk/c/host/host.c
)

set_target_properties(WebMfxHost PROPERTIES SUFFIX ".html")

target_compile_options(WebMfxHost PRIVATE
	-sMAIN_MODULE
)

set(DEBUG_LINK_OPTS
	-g -gsource-map
	--source-map-base ${ARG_SOURCE_MAP_BASE}
	-Wno-limited-postlink-optimizations
)

target_link_options(WebMfxHost PRIVATE
	-sMAIN_MODULE
	-sEXPORTED_RUNTIME_METHODS=cwrap,FS
	-sMIN_WEBGL_VERSION=2
	-sMAX_WEBGL_VERSION=2
	--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/html_templates/index.html
	--post-js ${bindingFilePrefix}.js
	$<$<CONFIG:Debug>:${DEBUG_LINK_OPTS}>
)

target_include_directories(WebMfxHost PRIVATE
	src/openmfx
	src/openmfx-sdk/c
	${bindingDir}
)

add_custom_target(Bindings DEPENDS "${bindingFilePrefix}.cpp")
add_dependencies(WebMfxHost Bindings)

# Additional

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/js/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/js/)
