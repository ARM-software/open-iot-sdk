/* Copyright (c) 2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwip/opt.h"

#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "netif/ppp/pppoe.h"

#include "lwip_memory_manager.h"
#include "lwip_stack.h"

#define US_TO_TICKS( xTimeInMs )    ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) * ( TickType_t ) configTICK_RATE_HZ ) / ( TickType_t ) 1000000U ) )

typedef struct pbuf pbuf_t;

extern mdh_emac_t *cs300_emac_get_default_instance(void);

#if LWIP_IGMP

#include "lwip/igmp.h"
/**
 * IPv4 address filtering setup.
 *
 * \param[in] netif the lwip network interface structure
 * \param[in] group IPv4 group to modify
 * \param[in] action
 * \return ERR_OK or error code
 */
static err_t emac_igmp_mac_filter(struct netif *netif, const ip4_addr_t *group, enum netif_mac_filter_action action)
{
    netif_context_t *netif_context_t = netif->state;

    switch (action) {
        case NETIF_DEL_MAC_FILTER:
        case NETIF_ADD_MAC_FILTER: {
            uint32_t group23 = ntohl(group->addr) & 0x007FFFFF;
            uint8_t addr[6];
            addr[0] = LL_IP4_MULTICAST_ADDR_0;
            addr[1] = LL_IP4_MULTICAST_ADDR_1;
            addr[2] = LL_IP4_MULTICAST_ADDR_2;
            addr[3] = group23 >> 16;
            addr[4] = group23 >> 8;
            addr[5] = group23;

            mdh_emac_status_t ret;
            if (action == NETIF_ADD_MAC_FILTER) {
                ret = mdh_emac_add_to_multicast_group(netif_context_t->emac, addr);
            } else {
                ret = mdh_emac_remove_from_multicast_group(netif_context_t->emac, addr);
            }

            return (ret == MDH_EMAC_STATUS_NO_ERROR) ? ERR_OK : ERR_IF;
        }
        default:
            return ERR_ARG;
    }
}

#endif

#if LWIP_IPV6_MLD

#include "lwip/mld6.h"
/**
 * IPv6 address filtering setup.
 *
 * \param[in] netif the lwip network interface structure
 * \param[in] group IPv6 group to modify
 * \param[in] action
 * \return ERR_OK or error code
 */
static err_t emac_mld_mac_filter(struct netif *netif, const ip6_addr_t *group, enum netif_mac_filter_action action)
{
    netif_context_t *netif_context_t = netif->state;

    switch (action) {
        case NETIF_DEL_MAC_FILTER:
        case NETIF_ADD_MAC_FILTER: {
            uint32_t group32 = ntohl(group->addr[3]);
            uint8_t addr[6];
            addr[0] = LL_IP6_MULTICAST_ADDR_0;
            addr[1] = LL_IP6_MULTICAST_ADDR_1;
            addr[2] = group32 >> 24;
            addr[3] = group32 >> 16;
            addr[4] = group32 >> 8;
            addr[5] = group32;

            mdh_emac_status_t ret;
            if (action == NETIF_ADD_MAC_FILTER) {
                ret = mdh_emac_add_to_multicast_group(netif_context_t->emac, addr);
            } else {
                ret = mdh_emac_remove_from_multicast_group(netif_context_t->emac, addr);
            }

            return (ret == MDH_EMAC_STATUS_NO_ERROR) ? ERR_OK : ERR_IF;
        }
        default:
            return ERR_ARG;
    }
}

#endif

/* this is called from ISR to let us know that input is ready*/
static void ethernetif_input(mdh_emac_t *self, void *ctx, mdh_emac_receive_t status)
{
    if (status == MDH_EMAC_RECEIVE_DONE) {
        netif_context_t *context = (netif_context_t *)(ctx);
        signal_receive(context);
    }
}

static err_t ethernetif_output(struct netif *netif, struct pbuf *p)
{
    netif_context_t *context = netif->state;
    mdh_network_stack_buffer_t *buf = (mdh_network_stack_buffer_t *)p;

    if (p->next) {
        /* if it's not a single chunk, allocate a new buf to hold the whole payload */
        mdh_network_stack_memory_manager_t *memory_manager = lwip_mm_get_instance();

        buf = mdh_network_stack_memory_manager_alloc_from_heap(
            memory_manager,
            mdh_network_stack_memory_manager_get_payload_len(memory_manager, buf),
            mdh_emac_get_align(context->emac)
        );

        if (!buf) {
            return ERR_IF;
        }

        mdh_network_stack_memory_manager_copy(memory_manager, (mdh_network_stack_buffer_t *)p, buf);
    } else {
        /* Increase reference counter since lwip stores handle to pbuf and frees it after output */
        pbuf_ref(p);
    }

    mdh_emac_status_t ret = mdh_emac_transmit(context->emac, buf);

    return (ret == MDH_EMAC_STATUS_NO_ERROR) ? ERR_OK : ERR_IF;
}

static void ethernetif_sent(mdh_emac_t *self, void *ctx, mdh_emac_transfer_t status, const mdh_network_stack_buffer_t *buffer)
{
    if (status == MDH_EMAC_TRANSFER_ERROR) {
        //@TODO react?
    }

    mdh_network_stack_memory_manager_free(lwip_mm_get_instance(), buffer);
}

static void emac_state_change(mdh_emac_t *self, void *ctx, mdh_emac_event_t event)
{
    /* this does not work because the handler fails to give us the context */
    netif_context_t *context = (netif_context_t *)(ctx);
    if (event == MDH_EMAC_EVENT_LINK_STATUS_CHANGE) {
        /* @TODO missing event types so we infer state based on netif state instead,
         * if we were down this means we just went up */
        if (netif_is_link_up(&context->lwip_netif)) {
            tcpip_callback_with_block((tcpip_callback_fn)netif_set_link_down, &context->lwip_netif, 1);
        } else {
            tcpip_callback_with_block((tcpip_callback_fn)netif_set_link_up, &context->lwip_netif, 1);
        }
    }
}

void ethernetif_process_input(netif_context_t *context)
{
    struct pbuf *buf = mdh_network_stack_memory_manager_alloc_from_heap(
        lwip_mm_get_instance(),
        mdh_emac_get_mtu(context->emac),
        mdh_emac_get_align(context->emac)
    );

    if (!buf) {
        LWIP_DEBUGF(NETIF_DEBUG, ("Emac LWIP: pbuf allocation failed in ethernetif_process_input\n"));
        return;
    }

    mdh_emac_receive(context->emac, buf);

    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (context->lwip_netif.input(buf, &context->lwip_netif) != ERR_OK) {
        mdh_network_stack_memory_manager_free(lwip_mm_get_instance(), buf);
    }
}

err_t ethernetif_init(struct netif *netif)
{
    int err = ERR_OK;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif_context_t *context = netif->state;
    mdh_emac_t *emac = cs300_emac_get_default_instance();
    context->emac = emac;

    /* Interface capabilities */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    mdh_network_stack_memory_manager_t *memory_manager = lwip_mm_get_instance();

    static const mdh_emac_callbacks_t cbks = {
        .rx = ethernetif_input, .tx = ethernetif_sent, .event = emac_state_change
    };

    if (MDH_EMAC_STATUS_NO_ERROR != mdh_emac_power_up(context->emac, &cbks, memory_manager, context)) {
        err = ERR_IF;
    }

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = 'e';
    netif->name[1] = 'n';

    netif->mtu = mdh_emac_get_mtu(emac);
    /* We have a default MAC address, so do don't force them to supply one */
    netif->hwaddr_len = mdh_emac_get_mac_addr_size(emac);
    /* They may or may not update hwaddr with their address */
    mdh_emac_get_mac_addr(emac, netif->hwaddr);
    /* Then we write back either what they gave us, or our default */
    mdh_emac_set_mac_addr(emac, netif->hwaddr);

#if LWIP_IPV4
    netif->output = etharp_output;
#if LWIP_IGMP
    netif->igmp_mac_filter = emac_igmp_mac_filter;
    netif->flags |= NETIF_FLAG_IGMP;
#endif /* LWIP_IGMP */
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#if LWIP_IPV6_MLD
    netif->mld_mac_filter = emac_mld_mac_filter;
    netif->flags |= NETIF_FLAG_MLD6;
#else
// Would need to enable all multicasts here - no API in fsl_enet to do that
#error "IPv6 multicasts won't be received if LWIP_IPV6_MLD is disabled, breaking the system"
#endif
#endif

    netif->linkoutput = ethernetif_output;

    return err;
}

/* Temporarily override definitions from MCU driver HAL as they don't work in NS
   mode */
void $Sub$$us_ticker_init(void)
{
}

void $Sub$$us_ticker_util_wait(uint32_t us)
{
    vTaskDelay(US_TO_TICKS(us));
}
