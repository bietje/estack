include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
SET (IP_SRCS main.c)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})

add_executable(ip-test ${IP_SRCS})
target_link_libraries(ip-test estack-static ${PCAP_LIBRARY})
