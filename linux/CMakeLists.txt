cmake_minimum_required(VERSION 3.5)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(command-daemon)

set(EXECUTABLE_OUTPUT_PATH "bin")
set(LIBRARY_OUTPUT_DIRECTORY "lib")

set(SOURCE_EXECUTABLE daemon.c)
set(SOURCE_LIBRARY command_receiver.c command_bram.c command_handler.c cos.c log.c)

#add_library(bram STATIC command_bram.c)
#add_library(handler STATIC command_handler.c)
#add_library(cos STATIC cos.c)

add_library(prj STATIC ${SOURCE_LIBRARY})

add_executable(command-daemon ${SOURCE_EXECUTABLE})

#target_link_libraries(command-daemon bram handler cos m)
target_link_libraries(command-daemon prj m)
