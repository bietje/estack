#
# E/STACK library CMake file
#

include( ${PROJECT_SOURCE_DIR}/cmake/pcap.cmake )
include (TestBigEndian)

include( ${PROJECT_SOURCE_DIR}/cmake/port.cmake )

SET(CONFIG_POLL_TMO CACHE STRING 100)

SET(ESTACK_SRCS
netbuf.c
util.c
netdev.c
translate.c
log.c
nif.c
addrutil.c
neighbour.c
ip.c
route.c
sock.c
socket.c
addr.c
init.c
ipv4/arp-in.c
ipv4/arp-out.c
ipv4/arp.c
ipv4/ip-output.c
ipv4/ip-input.c
ipv4/icmp.c
ipv4/frag.c
802.3/eth-in.c
802.3/eth-out.c
802.3/addr.c
transport/udp.c
${ESTACK_PORT_SRCS}
)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
SET(ESTACK_SRCS
${ESTACK_SRCS}
drivers/pcap.c
)
SET(HAVE_DEBUG True)
ENDIF()

IF(CMAKE_SYSTEM_NAME MATCHES Generic)
SET(ESTACK_SRCS
${ESTACK_SRCS}
sys.c)
ENDIF()

SET(${ESTACK_SRCS}
${ESTACK_SRCS}
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
addr.h
arp.h
compiler.h
compiler-gcc.h
compiler-vc.h
config.h.in
error.h
estack.h
ethernet.h
icmp.h
inet.h
ip.h
list.h
log.h
neighbour.h
netbuf.h
netdev.h
pcapdev.h
port.h
prototype.h
route.h
socket.h
test.h
translate.h
types.h
udp.h
)

SET(BASE_HDRS
${PROJECT_SOURCE_DIR}/include/estack.h
${ESTACK_PORT_HEADERS})

# Append include dir to header files
prepend_path(GENERIC_HEADERS ${INCLUDE_DIR}/estack)

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
SET(HAVE_BIG_ENDIAN True)
else()
SET(HAVE_BIG_ENDIAN False)
endif()

SET(HAVE_CI ${CMAKE_CI})

# Compile the configure file
CONFIGURE_FILE(${INCLUDE_DIR}/estack/config.h.in ${CMAKE_BINARY_DIR}/config.h)

# Set the include directories
include_directories(${PROJECT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR} ${ESTACK_PORT_INCLUDE_DIR})

# Shared and static library definitions
if(CMAKE_LIBTYPE MATCHES Shared)
  add_library(estack SHARED ${ESTACK_SRCS} ${GENERIC_HEADERS} ${BASE_HDRS})
	target_link_libraries(estack ${PCAP_LIBRARY})
endif()

add_library(estack-static STATIC ${ESTACK_SRCS} ${GENERIC_HEADERS} ${BASE_HDRS})

IF(CMAKE_BUILD_TYPE MATCHES Debug)
	target_link_libraries(estack-static ${PCAP_LIBRARY})
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)


# Generate MSVC filters
foreach(source IN LISTS ${ESTACK_SRCS})
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "Source Files\\${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()
