cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)

project(toy-8086)
add_executable(toy-8086
	./src/cpu.cc
	./src/main.cc)
