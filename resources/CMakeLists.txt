#
# Resources CMake file
#

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/arp.pcap ${CMAKE_BINARY_DIR}/resources/arp.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/icmp.pcap ${CMAKE_BINARY_DIR}/resources/icmp.pcap COPYONLY)
