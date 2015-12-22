cmake_minimum_required(VERSION 3.0)
if (WIN32)
    add_definitions(-DTOY8086_WIN32)
endif()
if (UNIX)
    add_definitions(-DTOY8086_UNIX)
    add_definitions(-std=gnu++11)
endif()
project(toy-8086)
add_executable(toy-8086
	./src/cpu.cc
	./src/main.cc)

if (CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=gnu++11)
endif()
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_definitions(-DTOY8086_MSVC)
endif()