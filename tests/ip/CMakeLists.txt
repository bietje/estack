include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
SET (IP_SRCS ip-test.c)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})

add_executable(ip-test ${IP_SRCS})
target_link_libraries(ip-test estack-static ${PCAP_LIBRARY})

add_executable(iproute-test iproute-test.c)
target_link_libraries(iproute-test estack-static ${PCAP_LIBRARY})

add_executable(icmp-test icmp-test.c)
target_link_libraries(icmp-test estack-static ${PCAP_LIBRARY})

add_executable(ipfrag-test ipfrag-test.c)
target_link_libraries(ipfrag-test estack-static ${PCAP_LIBRARY})

add_custom_target(run_ip
COMMAND ip-test resources/icmp-reply.pcap
DEPENDS ip-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(run_iproute
COMMAND iproute-test resources/icmp-reply.pcap
DEPENDS iproute-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(run_icmp
COMMAND icmp-test resources/icmp-reply.pcap
DEPENDS icmp-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(run_ipfrag
COMMAND ipfrag-test resources/icmp-reply.pcap
DEPENDS ipfrag-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
