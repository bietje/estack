include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
include (${PROJECT_SOURCE_DIR}/cmake/port.cmake)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR} ${ESTACK_PORT_INCLUDE_DIR})

add_executable(udp-test udp-test.c)
target_link_libraries(udp-test estack-static ${PCAP_LIBRARY} ${ESTACK_SYSTEM_LIBS})

add_executable(tcp-connectclose-test tcp-connect-close.c)
target_link_libraries(tcp-connectclose-test estack-static ${PCAP_LIBRARY} ${ESTACK_SYSTEM_LIBS})

add_executable(tcp-connect-test tcp-connect.c)
target_link_libraries(tcp-connect-test estack-static ${PCAP_LIBRARY} ${ESTACK_SYSTEM_LIBS})

add_custom_target(run_udptest
COMMAND ${CMAKE_CURRENT_BINARY_DIR}/udp-test resources/udp-input.pcap resources/dns-response.pcap
DEPENDS udp-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(run_tcpconnectclose
COMMAND ${CMAKE_CURRENT_BINARY_DIR}/tcp-connectclose-test resources/tcp/client/synack.pcap resources/tcp/client/finack-server.pcap resources/tcp/client/finack-server-ack.pcap
DEPENDS tcp-connectclose-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
