cmake_minimum_required(VERSION 3.5)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(mem-cpy)

set(EXECUTABLE_OUTPUT_PATH "bin")
set(LIBRARY_OUTPUT_DIRECTORY "lib")

set(SOURCE_EXECUTABLE main.c)

add_executable(${PROJECT_NAME} ${SOURCE_EXECUTABLE})
