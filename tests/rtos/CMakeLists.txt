include (${PROJECT_SOURCE_DIR}/cmake/pcap.cmake)
include( ${PROJECT_SOURCE_DIR}/cmake/port.cmake )
SET (ETH_TEST_SRCS main.c)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR} ${ESTACK_PORT_INCLUDE_DIR})

add_executable(rtos-test ${ETH_TEST_SRCS})
target_link_libraries(rtos-test estack-static ${PCAP_LIBRARY} ${ESTACK_SYSTEM_LIBS})

add_custom_target(run_rtos
COMMAND rtos-test resources/ip-fragments.pcap
DEPENDS rtos-test
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
