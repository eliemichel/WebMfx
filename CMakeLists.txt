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
