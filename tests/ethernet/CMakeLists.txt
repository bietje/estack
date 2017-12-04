SET (ETH_TEST_SRCS main.c)

include_directories(${PROJECT_SOURCE_DIR}/external/pcap/include ${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/external/pcap/lib)

add_executable(ethernet-test ${ETH_TEST_SRCS})
target_link_libraries(ethernet-test estack-static packet wpcap)