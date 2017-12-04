#
# E/STACK library CMake file
#

include( ${PROJECT_SOURCE_DIR}/cmake/pcap.cmake )

SET(ESTACK_SRCS
netbuf.c
util.c
netdev.c
drivers/pcap.c
802.3/eth-in.c
802.3/eth-out.c
)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
SET(${ESTACK_SRCS}
${ESTACK_SRCS}
${PROJECT_SOURCE_DIR}/source/dev/pcap.c
)
ENDIF()

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
ethernet.h
pcapdev.h
)

SET(BASE_HDRS
${PROJECT_SOURCE_DIR}/include/estack.h)

# Append include dir to header files
prepend_path(GENERIC_HEADERS ${INCLUDE_DIR}/estack)

# Compile the configure file
CONFIGURE_FILE(${INCLUDE_DIR}/estack/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Set the include directories
include_directories(${PROJECT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})

# Shared and static library definitions
add_library(estack SHARED ${ESTACK_SRCS} ${GENERIC_HEADERS} ${BASE_HDRS})
add_library(estack-static STATIC ${ESTACK_SRCS} ${GENERIC_HEADERS} ${BASE_HDRS})

IF(CMAKE_BUILD_TYPE MATCHES Debug)
	target_link_libraries(estack ${PCAP_LIBRARY})
	target_link_libraries(estack-static ${PCAP_LIBRARY})
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
