#
# E/STACK tools CMake file
#

IF(WIN32)
	SET(BOT_SRC byteordertool.c getopt.c getopt.h)
ELSE()
	SET(BOT_SRC byteordertool.c)
ENDIF()

include_directories(${CMAKE_CURRENT_PROJECT_DIR})
add_executable(byteordertool ${BOT_SRC})