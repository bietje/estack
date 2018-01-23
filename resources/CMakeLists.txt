#
# Resources CMake file
#

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/arp-request.pcap ${CMAKE_BINARY_DIR}/resources/arp-request.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/arp-reply.pcap ${CMAKE_BINARY_DIR}/resources/arp-reply.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/arp-reply-full.pcap ${CMAKE_BINARY_DIR}/resources/arp-reply-full.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/icmp.pcap ${CMAKE_BINARY_DIR}/resources/icmp.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/icmp-reply.pcap ${CMAKE_BINARY_DIR}/resources/icmp-reply.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/icmp-request.pcap ${CMAKE_BINARY_DIR}/resources/icmp-request.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/dns-response.pcap ${CMAKE_BINARY_DIR}/resources/dns-response.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ip-fragments.pcap ${CMAKE_BINARY_DIR}/resources/ip-fragments.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/udp-input.pcap ${CMAKE_BINARY_DIR}/resources/udp-input.pcap COPYONLY)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/tcp/client/synack.pcap ${CMAKE_BINARY_DIR}/resources/tcp/client/synack.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/tcp/client/finack.pcap ${CMAKE_BINARY_DIR}/resources/tcp/client/finack.pcap COPYONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/tcp/syn.pcap ${CMAKE_BINARY_DIR}/resources/tcp/syn.pcap COPYONLY)
