include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
SET (ARP_SRCS main.c)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})

add_executable(arp-test ${ARP_SRCS})
target_link_libraries(arp-test estack-static ${PCAP_LIBRARY})
