/*
 * Copyright (C) Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author Martine Lenders <mlenders@inf.fu-berlin.de>
 * @author Erik Ekman <eekman@google.com>
 */

#include "kernel_defines.h"

#include "lwip/tcpip.h"
#include "lwip/netif/netdev.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "netif/lowpan6.h"

#ifdef MODULE_AT86RF2XX
#include "at86rf2xx.h"
#include "at86rf2xx_params.h"
#endif

#ifdef MODULE_ENC28J60
#include "enc28j60.h"
#include "enc28j60_params.h"
#endif

#ifdef MODULE_MRF24J40
#include "mrf24j40.h"
#include "mrf24j40_params.h"
#endif

#ifdef MODULE_SOCKET_ZEP
#include "socket_zep.h"
#include "socket_zep_params.h"
#endif

#ifdef MODULE_SAM0_ETH
#include "sam0_eth_netdev.h"
#endif

#ifdef MODULE_STM32_ETH
#include "stm32_eth.h"
#endif

#ifdef MODULE_NRF802154
#include "nrf802154.h"
#endif

#if IS_USED(MODULE_NETDEV_IEEE802154_SUBMAC)
#include "net/netdev/ieee802154_submac.h"
#endif

#include "lwip.h"
#include "lwip_init_devs.h"

#define ENABLE_DEBUG    0
#include "debug.h"

#ifdef MODULE_AT86RF2XX
#define LWIP_NETIF_NUMOF        ARRAY_SIZE(at86rf2xx_params)
#endif

#ifdef MODULE_ENC28J60    /* is mutual exclusive with above ifdef */
#define LWIP_NETIF_NUMOF        ARRAY_SIZE(enc28j60_params)
#endif

#ifdef MODULE_MRF24J40     /* is mutual exclusive with above ifdef */
#define LWIP_NETIF_NUMOF        ARRAY_SIZE(mrf24j40_params)
#endif

#ifdef MODULE_SOCKET_ZEP   /* is mutual exclusive with above ifdef */
#define LWIP_NETIF_NUMOF        ARRAY_SIZE(socket_zep_params)
#endif

/* is mutual exclusive with above ifdef */
#ifdef MODULE_SAM0_ETH
#define LWIP_NETIF_NUMOF        (1)
#endif

#ifdef MODULE_STM32_ETH
#define LWIP_NETIF_NUMOF        (1)
#endif

#ifdef MODULE_NRF802154
#define LWIP_NETIF_NUMOF        (1)
#endif

#ifdef LWIP_NETIF_NUMOF
static struct netif netif[LWIP_NETIF_NUMOF];
#endif

#ifdef MODULE_AT86RF2XX
static at86rf2xx_t at86rf2xx_devs[LWIP_NETIF_NUMOF];
#endif

#ifdef MODULE_ENC28J60
static enc28j60_t enc28j60_devs[LWIP_NETIF_NUMOF];
#endif

#ifdef MODULE_MRF24J40
static mrf24j40_t mrf24j40_devs[LWIP_NETIF_NUMOF];
#endif

#ifdef MODULE_SOCKET_ZEP
static socket_zep_t socket_zep_devs[LWIP_NETIF_NUMOF];
#endif

#ifdef MODULE_SAM0_ETH
static netdev_t sam0_eth;
extern void sam0_eth_setup(netdev_t *netdev);
#endif

#ifdef MODULE_STM32_ETH
static netdev_t stm32_eth;
extern void stm32_eth_netdev_setup(netdev_t *netdev);
#endif

#ifdef MODULE_NRF802154
static netdev_ieee802154_submac_t nrf802154_netdev;
#endif

void lwip_bootstrap(void)
{
    lwip_netif_init_devs();
    /* TODO: do for every eligible netdev */
#ifdef LWIP_NETIF_NUMOF
#ifdef MODULE_MRF24J40
    for (unsigned i = 0; i < LWIP_NETIF_NUMOF; i++) {
        mrf24j40_setup(&mrf24j40_devs[i], &mrf24j40_params[i], i);
        if (netif_add_noaddr(&netif[i], &mrf24j40_devs[i].netdev.netdev, lwip_netdev_init,
                             tcpip_6lowpan_input) == NULL) {
            DEBUG("Could not add mrf24j40 device\n");
            return;
        }
    }
#elif defined(MODULE_AT86RF2XX)
    for (unsigned i = 0; i < LWIP_NETIF_NUMOF; i++) {
        at86rf2xx_setup(&at86rf2xx_devs[i], &at86rf2xx_params[i], i);
        if (netif_add_noaddr(&netif[i], &at86rf2xx_devs[i].netdev.netdev, lwip_netdev_init,
                             tcpip_6lowpan_input) == NULL) {
            DEBUG("Could not add at86rf2xx device\n");
            return;
        }
    }
#elif defined(MODULE_ENC28J60)
    for (unsigned i = 0; i < LWIP_NETIF_NUMOF; i++) {
        enc28j60_setup(&enc28j60_devs[i], &enc28j60_params[i], i);
        if (netif_add_noaddr(&netif[0], &enc28j60_devs[i].netdev, lwip_netdev_init,
                             tcpip_input) == NULL) {
            DEBUG("Could not add enc28j60 device\n");
            return;
        }
    }
#elif defined(MODULE_SOCKET_ZEP)
    for (unsigned i = 0; i < LWIP_NETIF_NUMOF; i++) {
        socket_zep_setup(&socket_zep_devs[i], &socket_zep_params[i], i);
        if (netif_add_noaddr(&netif[i], &socket_zep_devs[i].netdev.netdev, lwip_netdev_init,
                             tcpip_6lowpan_input) == NULL) {
            DEBUG("Could not add socket_zep device\n");
            return;
        }
    }
#elif defined(MODULE_SAM0_ETH)
    sam0_eth_setup(&sam0_eth);
    if (netif_add_noaddr(&netif[0], &sam0_eth, lwip_netdev_init,
                         tcpip_input) == NULL) {
        DEBUG("Could not add sam0_eth device\n");
        return;
    }
#elif defined(MODULE_STM32_ETH)
    stm32_eth_netdev_setup(&stm32_eth);
    if (netif_add_noaddr(&netif[0], &stm32_eth, lwip_netdev_init,
                         tcpip_input) == NULL) {
        DEBUG("Could not add stm32_eth device\n");
        return;
    }
#elif defined(MODULE_NRF802154)
    netdev_register(&nrf802154_netdev.dev.netdev, NETDEV_NRF802154, 0);
    netdev_ieee802154_submac_init(&nrf802154_netdev);

    nrf802154_hal_setup(&nrf802154_netdev.submac.dev);
    nrf802154_init();
    if (netif_add_noaddr(&netif[0], &nrf802154_netdev.dev.netdev, lwip_netdev_init,
                         tcpip_6lowpan_input) == NULL) {
        DEBUG("Could not add nrf802154 device\n");
        return;
    }
#endif
#endif
    /* also allow for external interface definition */
    tcpip_init(NULL, NULL);
#if IS_USED(MODULE_LWIP_DHCP_AUTO)
    {
        /* Start DHCP on all supported netifs. Interfaces that support
         * link status events will reset DHCP retries when link comes up. */
        struct netif *n = NULL;
        NETIF_FOREACH(n) {
            if (netif_is_flag_set(n, NETIF_FLAG_ETHERNET)) {
                netifapi_dhcp_start(n);
            }
        }
    }
#endif
}

/** @} */
