# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

# Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

include(lwip-fetch.cmake)

add_library(lwip)

target_include_directories(lwip
    PUBLIC
        ${lwip_SOURCE_DIR}/src/include
        ${lwip_SOURCE_DIR}/src/include/netif
        ${lwip_SOURCE_DIR}/src/include/lwip
)

target_sources(lwip
    PRIVATE
        ${lwip_SOURCE_DIR}/src/api/api_lib.c
        ${lwip_SOURCE_DIR}/src/api/api_msg.c
        ${lwip_SOURCE_DIR}/src/api/err.c
        ${lwip_SOURCE_DIR}/src/api/netbuf.c
        ${lwip_SOURCE_DIR}/src/api/netdb.c
        ${lwip_SOURCE_DIR}/src/api/netifapi.c
        ${lwip_SOURCE_DIR}/src/api/sockets.c
        ${lwip_SOURCE_DIR}/src/api/tcpip.c
        ${lwip_SOURCE_DIR}/src/core/def.c
        ${lwip_SOURCE_DIR}/src/core/dns.c
        ${lwip_SOURCE_DIR}/src/core/inet_chksum.c
        ${lwip_SOURCE_DIR}/src/core/init.c
        ${lwip_SOURCE_DIR}/src/core/mem.c
        ${lwip_SOURCE_DIR}/src/core/memp.c
        ${lwip_SOURCE_DIR}/src/core/netif.c
        ${lwip_SOURCE_DIR}/src/core/raw.c
        ${lwip_SOURCE_DIR}/src/core/stats.c
        ${lwip_SOURCE_DIR}/src/core/sys.c
        ${lwip_SOURCE_DIR}/src/core/tcp.c
        ${lwip_SOURCE_DIR}/src/core/tcp_in.c
        ${lwip_SOURCE_DIR}/src/core/timeouts.c
        ${lwip_SOURCE_DIR}/src/core/udp.c
        ${lwip_SOURCE_DIR}/src/core/pbuf.c
        ${lwip_SOURCE_DIR}/src/core/tcp_out.c
        ${lwip_SOURCE_DIR}/src/core/ip.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/acd.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/autoip.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/dhcp.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/icmp.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/igmp.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/ip4.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/ip4_addr.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/ip4_frag.c
        ${lwip_SOURCE_DIR}/src/core/ipv4/etharp.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/dhcp6.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/ethip6.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/icmp6.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/inet6.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/ip6.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/ip6_addr.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/ip6_frag.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/mld6.c
        ${lwip_SOURCE_DIR}/src/core/ipv6/nd6.c
        ${lwip_SOURCE_DIR}/src/netif/ethernet.c
)

target_link_libraries(lwip
    PRIVATE
        lib-bsp
        freertos-kernel
)
