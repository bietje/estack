include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
SET (ETH_TEST_SRCS main.c)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})

add_executable(netdev-test ${ETH_TEST_SRCS})
target_link_libraries(netdev-test estack-static ${PCAP_LIBRARY})
