/* Copyright (c) 2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWIP_EMAC_NETIF_H_
#define LWIP_EMAC_NETIF_H_

#include <stdbool.h>
#include "lwip/opt.h"

#include "lwip/def.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "ethernet.h"
#include "emac_api.h"

typedef enum connection_status_t {
    CONNECTION_STATUS_DISCONNECTED,
    CONNECTION_STATUS_CONNECTING,
    CONNECTION_STATUS_GLOBAL_UP,
    CONNECTION_STATUS_LOCAL_UP
} connection_status_t;

typedef enum address_status_t {
    ADDRESS_STATUS_HAS_ANY_ADDR = 1,
    ADDRESS_STATUS_HAS_PREF_ADDR = 2,
    ADDRESS_STATUS_HAS_BOTH_ADDR = 4
} address_status_t;

typedef struct netif_context_t {
    mdh_emac_t *emac;
    struct netif lwip_netif;
    QueueHandle_t receive_semaphore;
    connection_status_t connected;
    bool dhcp_has_to_be_set;
    bool dhcp_started;
    uint8_t has_addr_state;
} netif_context_t;

err_t ethernetif_init(struct netif *netif);
void ethernetif_process_input(netif_context_t *netif);

#endif // LWIP_EMAC_NETIF_H_
