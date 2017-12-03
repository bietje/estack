#
# E/STACK root CMake file
#
# Author: Michel Megens
# Email:  dev@bietje.net
#

cmake_minimum_required(VERSION 3.0)
project(estack)

include(CheckIncludeFiles)

if(UNIX)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
endif(UNIX)

# Check header files
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (stdio.h  HAVE_STDIO_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (stdarg.h HAVE_STDARG_H)
CHECK_INCLUDE_FILES (assert.h HAVE_ASSERT_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (Winsock2.h HAVE_WINSOCK_H)
CHECK_INCLUDE_FILES (arpa/inet.h HAVE_INET_H)

add_subdirectory(source)
add_subdirectory(resources)
