/* Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AZURE_NETX_CONFIG_H
#define AZURE_NETX_CONFIG_H

#include "nx_api.h"

#define PACKET_COUNT          32
#define PACKET_SIZE           1536
#define SAMPLE_POOL_SIZE      ((PACKET_SIZE + sizeof(NX_PACKET)) * PACKET_COUNT)
#define SAMPLE_ARP_CACHE_SIZE 512

#define SNTP_MAX_SYNC_CHECK        30
#define SNTP_MAX_UPDATE_CHECK      10
#define SNTP_UPDATE_SLEEP_INTERVAL (NX_IP_PERIODIC_RATE / 2)
#define DHCP_WAIT_TIME             (20 * NX_IP_PERIODIC_RATE)

// GMT: Friday, Jan 1, 2022 12:00:00 AM. Epoch timestamp: 1640995200.
#define SAMPLE_SYSTEM_TIME 1640995200

// Seconds between Unix Epoch (1/1/1970) and NTP Epoch (1/1/1999)
#define SAMPLE_UNIX_TO_NTP_EPOCH_SECOND 0x83AA7E80

#endif // AZURE_NETX_CONFIG_H
