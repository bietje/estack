#
# E/STACK root CMake file
#
# Author: Michel Megens
# Email:  dev@bietje.net
#

cmake_minimum_required(VERSION 3.0)
project(estack)

include(CMakeToolsHelpers OPTIONAL)
include(CheckIncludeFiles)
include(CheckTypeSize)

find_package(Doxygen)

if(UNIX OR CMAKE_SYSTEM_NAME MATCHES Generic)
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		SET(CFLAGS_EXTRA "-ggdb -g3")
	endif()

	if(BUILD_TYPE MATCHES Native)
		SET(CFLAGS_EXTRA "-march=native -mtune=native")
	endif()

	if(CMAKE_BUILD_TYPE MATCHES Release)
		set(CFLAGS_EXTRA "${CFLAGS_EXTRA} -ffast-math")
	endif()

	if(CMAKE_BUILD_TYPE MATCHES RelSize)
		set(CFLAGS_EXTRA "${CFLAGS_EXTRA} -Os")
	endif()

	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CFLAGS_EXTRA} -std=gnu11 -Wall")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS_EXTRA} -std=gnu++11 -Wall")
elseif(WIN32)
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		SET(CFLAGS_EXTRA "/W4 /Od /Ob1 /Zi /Gd")
	elseif(CMAKE_BUILD_TYPE MATCHES Release)
		SET(CFLAGS_EXTRA "/W4 /O2 /Ob2 /Gd /Oi")
	endif()
	
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CFLAGS_EXTRA}")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS_EXTRA}")
endif()


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

CHECK_TYPE_SIZE(size_t HAVE_SIZE_T)
CHECK_TYPE_SIZE(ssize_t HAVE_SSIZE_T)

if(CMAKE_SYSTEM_NAME MATCHES Generic)
SET(HAVE_GENERIC_SYS True)
endif()

add_subdirectory(source)
add_subdirectory(resources)

if( NOT CMAKE_SYSTEM_NAME MATCHES Generic )
add_subdirectory(tools)
endif()

IF(CMAKE_BUILD_TYPE MATCHES Debug)
add_subdirectory(tests)
ENDIF()

configure_file(${CMAKE_SOURCE_DIR}/Doxyfile ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/utest.yaml ${CMAKE_BINARY_DIR}/utest.yaml COPYONLY)
