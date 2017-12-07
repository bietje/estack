#
# Resources CMake file
#

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/arp-request.pcap ${CMAKE_BINARY_DIR}/resources/arp-request.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/arp-reply.pcap ${CMAKE_BINARY_DIR}/resources/arp-reply.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/icmp.pcap ${CMAKE_BINARY_DIR}/resources/icmp.pcap COPYONLY)
