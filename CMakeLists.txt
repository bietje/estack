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
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		SET(CFLAGS_EXTRA "-Og -ggdb -g3")
	endif()

	if(BUILD_TYPE MATCHES Native)
		SET(CFLAGS_EXTRA "-march=native -mtune=native")
	endif()

	if(CMAKE_BUILD_TYPE MATCHES Release)
		set(CFLAGS_EXTRA "${CFLAGS_EXTRA} -O3 -ffast-math")
	endif()

		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CFLAGS_EXTRA} -std=gnu11 -Wall")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS_EXTRA} -std=gnu++11 -Wall")
endif(UNIX)


# Check header files
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (stdio.h  HAVE_STDIO_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (stdarg.h HAVE_STDARG_H)
CHECK_INCLUDE_FILES (assert.h HAVE_ASSERT_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (time.h HAVE_TIME_H)
CHECK_INCLUDE_FILES (Winsock2.h HAVE_WINSOCK_H)
CHECK_INCLUDE_FILES (arpa/inet.h HAVE_INET_H)

add_subdirectory(source)
add_subdirectory(resources)
add_subdirectory(tools)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
add_subdirectory(tests)
ENDIF()
