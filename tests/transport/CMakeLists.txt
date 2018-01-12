include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
include (${PROJECT_SOURCE_DIR}/cmake/port.cmake)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR} ${ESTACK_PORT_INCLUDE_DIR})

add_executable(udp-output-test udp-output.c)
target_link_libraries(udp-output-test estack-static ${PCAP_LIBRARY} ${ESTACK_SYSTEM_LIBS})

add_custom_target(run_udp_output_test
COMMAND udp-test
DEPENDS udp-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})