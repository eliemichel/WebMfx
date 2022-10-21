cmake_minimum_required(VERSION 3.24...3.24)

project(
	WebMfx
	VERSION 0.1
	LANGUAGES C CXX)

include(cmake/WebMfx.cmake)

# Plugins

set(PLUGIN_C_SDK_SRC
	src/openmfx-sdk/c/common/common.c
	src/openmfx-sdk/c/plugin/attribute.c
	src/openmfx-sdk/c/plugin/mesh.c
)

add_webmfx_library(
	BoxPlugin
	SRC
		src/BoxPlugin.c
		${PLUGIN_C_SDK_SRC}
	INCLUDE
		src/openmfx
)

add_webmfx_library(
	ComputeNormalsPlugin
	SRC
		src/ComputeNormalsPlugin.c
		${PLUGIN_C_SDK_SRC}
	INCLUDE
		src/openmfx
)

# Host

add_emscripten_executable(
	WebMfxHost
	SRC
		src/webmfx.cpp
		src/openmfx-sdk/c/common/common.c
		src/openmfx-sdk/c/host/types.c
		src/openmfx-sdk/c/host/meshEffectSuite.c
		src/openmfx-sdk/c/host/propertySuite.c
		src/openmfx-sdk/c/host/parameterSuite.c
		src/openmfx-sdk/c/host/host.c
	INCLUDE
		src/openmfx
		src/openmfx-sdk/c
	BINDINGS
		src/binding.idl
	COMPILE_SETTINGS
		MAIN_MODULE
	LINK_SETTINGS
		MAIN_MODULE
		EXPORTED_RUNTIME_METHODS=cwrap,FS
		MIN_WEBGL_VERSION=2
		MAX_WEBGL_VERSION=2
	SHELL_FILE
		src/html_templates/index.html
)
set_target_properties(WebMfxHost PROPERTIES SUFFIX ".html")

# Additional

execute_process(
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	${CMAKE_CURRENT_BINARY_DIR}/css
	${CMAKE_CURRENT_SOURCE_DIR}/css
)
execute_process(
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	${CMAKE_CURRENT_BINARY_DIR}/js
	${CMAKE_CURRENT_SOURCE_DIR}/js
)
