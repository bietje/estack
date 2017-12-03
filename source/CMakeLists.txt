#
# E/STACK library CMake file
#

IF(WIN32)
	IF(CMAKE_BUILD_TYPE MATCHES Debug)
		SET(ESTACK_SOURCES_DBG ${CMAKE_CURRENT_SOURCE_DIR}/dev/pcap.c)
		link_directories(${PROJECT_SOURCE_DIR}/external/pcap/lib)
		include_directories(${PROJECT_SOURCE_DIR}/external/pcap/include)
	ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
ENDIF(WIN32)

SET(ESTACK_SRCS
netbuf.c
util.c
#${ESTACK_SOURCES_DBG}
)

# Magic macro's
FUNCTION( prepend_path SOURCE_FILES INC_PATH )
  FOREACH( SOURCE_FILE ${${SOURCE_FILES}} )
    SET( MODIFIED ${MODIFIED} ${INC_PATH}/${SOURCE_FILE} )
  ENDFOREACH()
  SET( ${SOURCE_FILES} ${MODIFIED} PARENT_SCOPE )
ENDFUNCTION()

SET(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
SET(GENERIC_HEADERS
estack.h
netbuf.h
list.h
config.h.in
netdev.h
)

# Append include dir to header files
prepend_path(GENERIC_HEADERS ${INCLUDE_DIR}/estack)

# Compile the configure file
CONFIGURE_FILE(${INCLUDE_DIR}/estack/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Set the include directories
include_directories(${PROJECT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})

# Shared and static library definitions
add_library(estack SHARED ${ESTACK_SRCS} ${GENERIC_HEADERS})
add_library(estack-static STATIC ${ESTACK_SRCS} ${GENERIC_HEADERS})

IF(CMAKE_BUILD_TYPE MATCHES Debug)
	IF(WIN32)
		target_link_libraries(estack packet wpcap)
		target_link_libraries(estack-static packet wpcap)
	ELSE()
		target_link_libraries(estack packet pcap)
		target_link_libraries(estack-static packet pcap)
	ENDIF()
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
