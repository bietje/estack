include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
include (${PROJECT_SOURCE_DIR}/cmake/port.cmake)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR} ${ESTACK_PORT_INCLUDE_DIR})

add_executable(udp-test udp-test.c)
target_link_libraries(udp-test estack-static ${PCAP_LIBRARY} ${ESTACK_SYSTEM_LIBS})

add_custom_target(run_udptest
COMMAND udp-test resources/udp-input.pcap resources/dns-response.pcap
DEPENDS udp-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
