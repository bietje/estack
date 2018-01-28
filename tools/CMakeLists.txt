#
# E/STACK tools CMake file
#

IF(WIN32)
	SET(BOT_SRC byteordertool.c getopt.c getopt.h)
ELSE()
	SET(BOT_SRC byteordertool.c)
ENDIF()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(byteordertool ${BOT_SRC})

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/tools/ci-runner.py ${CMAKE_BINARY_DIR}/tools/ci-runner.py COPYONLY)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/tools/utest.py ${CMAKE_BINARY_DIR}/tools/utest.py COPYONLY)
