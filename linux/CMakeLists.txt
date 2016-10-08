cmake_minimum_required(VERSION 3.0)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(ns7-control)

set(EXECUTABLE_OUTPUT_PATH "bin")
set(LIBRARY_OUTPUT_DIRECTORY "lib")

set(SOURCE_EXECUTABLE main.c)
set(SOURCE_LIBRARY command_receiver.c command_bram.c command_handler.c log.c gpio.c bin_file_loader.c)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG -D DEBUG=3"}")
add_definitions(-DDEBUG=1)

#add_library(bram STATIC command_bram.c)
#add_library(handler STATIC command_handler.c)
#add_library(cos STATIC cos.c)

add_library(prj STATIC ${SOURCE_LIBRARY})

add_executable(ns7-control ${SOURCE_EXECUTABLE})

target_link_libraries(ns7-control prj m)
