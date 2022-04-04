/* Copyright (c) 2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "FreeRTOS.h"
#include "lwipopts.h"
#include "semphr.h"
#include "lwip/err.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip_emac_netif.h"
#include "lwip_memory_manager.h"
#include "lwip_stack.h"
#include "FreeRTOS_IP.h"

/* secure sockets requires errno to communicate error codes */
int errno = 0;

static err_t set_dhcp(struct netif *netif)
{
    netif_context_t *context = (netif_context_t *)(netif->state);

#if LWIP_DHCP
    if (context->dhcp_has_to_be_set) {
        err_t err = dhcp_start(netif);
        if (err) {
            context->connected = CONNECTION_STATUS_DISCONNECTED;
            return ERR_IF;
        }

        context->dhcp_has_to_be_set = false;
        context->dhcp_started = true;
    }
#endif

    return ERR_OK;
}

static const ip_addr_t *get_ipv4_addr(const struct netif *netif)
{
#if LWIP_IPV4
    if (!netif_is_up(netif)) {
        return NULL;
    }

    if (!ip4_addr_isany(netif_ip4_addr(netif))) {
        return netif_ip_addr4(netif);
    }
#endif
    return NULL;
}

static const ip_addr_t *get_ipv6_addr(const struct netif *netif)
{
#if LWIP_IPV6
    if (!netif_is_up(netif)) {
        return NULL;
    }

    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
                !ip6_addr_islinklocal(netif_ip6_addr(netif, i))) {
            return netif_ip_addr6(netif, i);
        }
    }

    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
            return netif_ip_addr6(netif, i);
        }
    }
#endif
    return NULL;
}

static const ip_addr_t *get_ip_addr(bool any_addr, const struct netif *netif)
{
    const ip_addr_t *pref_ip_addr = 0;
    const ip_addr_t *npref_ip_addr = 0;

#if LWIP_IPV4 && LWIP_IPV6
#if IP_VERSION_PREF == PREF_IPV4
    pref_ip_addr = get_ipv4_addr(netif);
    npref_ip_addr = get_ipv6_addr(netif);
#else
    pref_ip_addr = get_ipv6_addr(netif);
    npref_ip_addr = get_ipv4_addr(netif);
#endif
#elif LWIP_IPV6
    pref_ip_addr = get_ipv6_addr(netif);
#elif LWIP_IPV4
    pref_ip_addr = get_ipv4_addr(netif);
#endif

    if (pref_ip_addr) {
        return pref_ip_addr;
    } else if (npref_ip_addr && any_addr) {
        return npref_ip_addr;
    }

    return NULL;
}

static void netif_link_irq(struct netif *netif)
{
    netif_context_t *context = (netif_context_t *)(netif->state);

    if (netif_is_link_up(netif) && context->connected == CONNECTION_STATUS_CONNECTING) {
        netif_set_up(netif);
    } else {
        if (netif_is_up(netif)) {
            context->connected = CONNECTION_STATUS_CONNECTING;
        }
        netif_set_down(netif);
    }
}

extern void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent );

static void netif_status_irq(struct netif *netif)
{
    netif_context_t *context = (netif_context_t *)(netif->state);

    if (netif_is_up(netif) && netif_is_link_up(netif)) {
        if (context->dhcp_has_to_be_set) {
            set_dhcp(netif);
        } else {
            if (!(context->has_addr_state & ADDRESS_STATUS_HAS_ANY_ADDR) && get_ip_addr(true, netif)) {
                context->has_addr_state |= ADDRESS_STATUS_HAS_ANY_ADDR;
            }
#if PREF_ADDR_TIMEOUT
            if (!(context->has_addr_state & ADDRESS_STATUS_HAS_PREF_ADDR) && get_ip_addr(false, netif)) {
                context->has_addr_state |= ADDRESS_STATUS_HAS_PREF_ADDR;
            }
#endif
#if BOTH_ADDR_TIMEOUT
            if (!(context->has_addr_state & ADDRESS_STATUS_HAS_BOTH_ADDR) && get_ipv4_addr(netif) && get_ipv6_addr(netif)) {
                context->has_addr_state |= ADDRESS_STATUS_HAS_BOTH_ADDR;
            }
#endif
            if (context->has_addr_state & ADDRESS_STATUS_HAS_ANY_ADDR) {
                context->connected = CONNECTION_STATUS_GLOBAL_UP;
#if LWIP_IPV6
                if (ip_addr_islinklocal(get_ipv6_addr(netif))) {
                    context->connected = CONNECTION_STATUS_LOCAL_UP;
                }
#endif
                vApplicationIPNetworkEventHook(eNetworkUp);
            }
        }
    } else if (!netif_is_up(netif) && netif_is_link_up(netif)) {
        context->connected = CONNECTION_STATUS_DISCONNECTED;

        vApplicationIPNetworkEventHook(eNetworkDown);
    }
}

static err_t ethernet_interface_bringup(netif_context_t *context)
{
    if (context->connected == CONNECTION_STATUS_GLOBAL_UP) {
        return ERR_ISCONN;
    } else if (context->connected == CONNECTION_STATUS_CONNECTING) {
        return ERR_INPROGRESS;
    }

    context->connected = CONNECTION_STATUS_CONNECTING;
    context->dhcp_has_to_be_set = true;

    return ERR_OK;
}

static err_t ethernet_interface_bringdown(netif_context_t *context)
{
    // Check if we've connected
    if (context->connected == CONNECTION_STATUS_DISCONNECTED) {
        return ERR_CONN;
    }

#if LWIP_DHCP
    // Disconnect from the network
    if (context->dhcp_started) {
        dhcp_release(&context->lwip_netif);
        dhcp_stop(&context->lwip_netif);
        context->dhcp_started = false;
        context->dhcp_has_to_be_set = false;
    }
#endif

    netif_set_down(&context->lwip_netif);

#if LWIP_IPV6
    mbed_lwip_clear_ipv6_addresses(&context->lwip_netif);
#endif
#if LWIP_IPV4
    ip_addr_set_zero(&(context->lwip_netif.ip_addr));
    ip_addr_set_zero(&(context->lwip_netif.netmask));
    ip_addr_set_zero(&(context->lwip_netif.gw));
#endif

    context->connected = CONNECTION_STATUS_DISCONNECTED;

    return ERR_OK;
}

static err_t ethernet_interface_init(netif_context_t *context)
{
    struct netif *n = netif_add(&context->lwip_netif,
#if LWIP_IPV4
                                0,
                                0,
                                0,
#endif
                                context /* state */,
                                &ethernetif_init,
                                tcpip_input);

    if (!n) {
        return ERR_IF;
    }

    netif_set_link_callback(&context->lwip_netif, &netif_link_irq);
    netif_set_status_callback(&context->lwip_netif, &netif_status_irq);

    netif_set_default(&context->lwip_netif);

    return ERR_OK;
}

static void tcpip_init_done(void *arg)
{
    SemaphoreHandle_t xSemaphore = *((SemaphoreHandle_t *)arg);
    xSemaphoreGive(xSemaphore);
}

static err_t lwip_network_init(netif_context_t *context)
{
    // it's safe to create them on the stack since they are only used until we exit this function
    StaticSemaphore_t semaphore_buffer;
    SemaphoreHandle_t semaphore;

    semaphore = xSemaphoreCreateBinaryStatic(&semaphore_buffer);

    /* Initialise LwIP, providing the callback function and callback semaphore */
    tcpip_init(tcpip_init_done, &semaphore);

    xSemaphoreTake(semaphore, portMAX_DELAY);

    return ERR_OK;
}

void signal_receive(netif_context_t *context)
{
    xQueueGiveFromISR(context->receive_semaphore, NULL);
}

void net_task(void *pvParameters)
{
    printf("Net task started\r\n");

    netif_context_t context = { 0 };

    context.receive_semaphore = xQueueCreate(1, 0);

    lwip_network_init(&context);
    ethernet_interface_init(&context);
    ethernet_interface_bringup(&context);

    /* @TODO: emac is missing this event so we need to trigger it ourselves*/
    tcpip_callback_with_block((tcpip_callback_fn)netif_set_link_up, &context.lwip_netif, 1);

    /* Initialise the RTOS's TCP/IP stack. The tasks that use the network are created in the
     * vApplicationIPNetworkEventHook() The hook function is called when the network connects. */

    while (1)
    {
        /* tick lwip */
        sys_check_timeouts();

        /* handle input */
        if (xQueueSemaphoreTake(context.receive_semaphore, 0)) {
            ethernetif_process_input(&context);
        }
    }
}
