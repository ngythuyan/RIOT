/*
 * SPDX-FileCopyrightText: 2015 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Child node of Energy experiment
 *
 * @author      Thuy An Nguyen
 *
 * @}
 */

#include "../experiment.h"
#include "random.h"
#include "macros/utils.h"
#include "net/utils.h"
//#include "mrhof_energy.h"

// for neighbor stats
#include "net/netstats.h"
#include "net/netstats/neighbor.h"

// for battery reading
#include "battery.h"

#define HIGH_ENERGY_CONSUMPTION (0)

#include "board.h"
#include "saul_reg.h"
#if HIGH_ENERGY_CONSUMPTION
    #include "ws281x.h"
    #include "ws281x_params.h"
    static ws281x_t dev;
#endif

#ifndef RECORD_CACHE_SIZE
#  define RECORD_CACHE_SIZE (15)
#endif

static uint32_t seq_no = 0;
static sock_udp_t sock;

static uint8_t buf_tx[40];
static msg_ping_t *ping = (void *)buf_tx;

static void _print_addr(void) 
{
    /* print all IPv6 addresses */
    printf("{\"IPv6 addresses\": [\"");
    netifs_print_ipv6("\", \"");
    puts("\"]}");
}

static int get_parent(char* parent)
{
    gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(INSTANCE_ID_DEFAULT);
    if (!inst || !inst->dodag.parents) {
        return - 1;
    }
    ipv6_addr_t dodag_id = inst->dodag.parents->addr;

    ipv6_to_identifier(&dodag_id, parent);
    return 0;
}

static int get_stats(void)
{
    gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(INSTANCE_ID_DEFAULT);
    if (!inst || !inst->dodag.parents) {
        return - 1;
    }
    gnrc_rpl_dodag_t *dodag = &inst->dodag;
    uint16_t rank = dodag->my_rank;
    ipv6_addr_t parent_addr = dodag->parents->addr;
    kernel_pid_t iface_pid = dodag->iface;

    /* Hop Count*/
    ping->hp = (rank / CONFIG_GNRC_RPL_DEFAULT_MIN_HOP_RANK_INCREASE) - 1;
    
    /* ETX and RSSI */
    gnrc_ipv6_nib_nc_t nce;
    gnrc_netif_t *iface = gnrc_netif_get_by_pid(iface_pid);
    if (gnrc_ipv6_nib_get_next_hop_l2addr(&parent_addr, iface, NULL, &nce)) {
        printf("Node: stats not found\n");
        ping->etx = 0;
        ping->rssi = 0;
        ping->lqi = 0;
        return 0;
    }

    netstats_nb_t nb_stats;
    netstats_nb_get(&iface->netif, nce.l2addr, nce.l2addr_len, &nb_stats);
    ping->etx = nb_stats.etx;
    ping->rssi = nb_stats.rssi;
    ping->lqi = nb_stats.lqi;
    return 0;
}

/* ================= Main ================= */
int main(void)
{
    /* ================= Init Node ================= */
    ztimer_sleep(ZTIMER_SEC, 2);

    /* Get first network interface */
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    if (!netif) {
        puts("Node: No network interface found");
        return 1;
    }
    printf("Node: Network interface PID: %d\n", netif->pid);

    puts("Node: Starting experiment"); 

    /* start MRHOF Energy */
//    init_mrhof_energy();

    /* init RPL */
    gnrc_rpl_init(netif->pid);
    puts("Node: Wait for parent");
    while (true) {
        printf("Node: waiting for parent\n");
        _print_addr();
        gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(INSTANCE_ID_DEFAULT);
        if (inst && inst->dodag.parents) {
            break;
        }
        ztimer_sleep(ZTIMER_SEC, 2);
    }
    char my_parent[5];
    get_parent(my_parent);
    printf("Found my parent: %s\n", my_parent);

    /* UDP setup */
    sock_udp_ep_t local = { .family = AF_INET6,
                            .netif = SOCK_ADDR_ANY_NETIF,
                            .port = PORT_DEFAULT };
    /* prepare udp endpoint*/
    sock_udp_ep_t remote = { 0 };
    if (sock_udp_str2ep(&remote, SERVER_DEFAULT) < 0) {
        puts("Send: Unable to parse destination address");
    }

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Node: Error creating UDP sock");
        return 1;
    }

    puts("Node: RPL and UDP setup finished");
    _print_addr();

    /* turn on LEDs */
    LED1_ON;
#if HIGH_ENERGY_CONSUMPTION
    LED0_ON;
    LED1_ON;
    ws281x_init (&dev, &ws281x_params[0]);
    color_rgb_t color = {255, 255, 255};
    ws281x_set (&dev, 0, color);
    ws281x_write (&dev);
#endif

    ztimer_sleep(ZTIMER_SEC, 30);
    puts("Send: sending thread start");
    uint32_t extra =  random_uint32_range (0, 200);
    while (1) {
        /* prepare ping message */
        int res = get_parent(ping->parent);
        if(res < 0) {
            ztimer_sleep(ZTIMER_USEC, delay_us);
            continue;
        }
        res = get_stats();
        if (res < 0) {
            ztimer_sleep(ZTIMER_USEC, delay_us);
            continue;
        }
        ping->energy = get_voltage();
        ping->msg_no = seq_no;
        msg_ping_t local_ping = *ping;

        /* send UDP msg */
        if((res = sock_udp_send(&sock, &local_ping, sizeof(msg_ping_t), &remote)) < 0) {
            puts("Send: could not send");
        }

        seq_no++;
        ztimer_sleep(ZTIMER_USEC, delay_us + extra);
    }
    
    return 0;
}
