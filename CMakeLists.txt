cmake_minimum_required(VERSION 3.24...3.24)

project(
	WebMfx
	VERSION 0.1
	LANGUAGES C CXX)

include(cmake/WebMfx.cmake)

# Plugins

add_webmfx_library(
	ExamplePlugin1
	SRC
		src/plugin.c
		src/openmfx-sdk/c/common/common.c
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

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/js/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/js/)
# mklink ${CMAKE_CURRENT_BINARY_DIR}/css ${CMAKE_CURRENT_SOURCE_DIR}/css
