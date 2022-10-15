cmake_minimum_required(VERSION 3.11...3.20)

project(
	WebMfx
	VERSION 0.1
	LANGUAGES C CXX)

add_executable(ExamplePlugin1 src/plugin.c)
target_include_directories(ExamplePlugin1 PRIVATE src/openmfx)

message(STATUS "You may now build the project by calling ${CMAKE_MAKE_PROGRAM}")
