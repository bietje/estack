include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
include( ${PROJECT_SOURCE_DIR}/cmake/port.cmake )
SET (ARP_SRCS main.c)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR} ${ESTACK_PORT_INCLUDE_DIR})

add_executable(arp-test ${ARP_SRCS})
target_link_libraries(arp-test estack-static ${PCAP_LIBRARY} ${ESTACK_SYSTEM_LIBS})

add_custom_target(run_arp
COMMAND arp-test ${PROJECT_BINARY_DIR}/resources/arp-request.pcap
DEPENDS arp-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
