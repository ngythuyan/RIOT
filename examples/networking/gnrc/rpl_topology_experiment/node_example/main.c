/*
 * SPDX-FileCopyrightText: 2015 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Showing minimum memory footprint of gnrc network stack
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include "ztimer.h"
#include "thread.h"
#include <inttypes.h>

#include "msg.h"
#include "net/sock/udp.h"
#include "net/sock/util.h"
#include "net/sock/async/event.h"
#include "net/ipv6/addr.h"

#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/netif/hdr.h"
#include "net/ipv6/addr.h"
#include "net/netif.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/rpl/dodag.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/pktdump.h"
#include "net/gnrc/rpl.h"

#include "random.h"
#include "utlist.h"

// for neighbor stats
#include "net/netstats.h"
#include "net/netstats/neighbor.h"

#define RPL_IID                 (0x1)
#define RPL_PREFIX_LEN          (64U)
#define MAIN_QUEUE_SIZE     (8)
#define NUM_OF_PINGS (1000)

#define MAIN_QUEUE_SIZE 32
#define IPV6_CUSTOM_ADDR_STR_LEN (5)
#define INSTANCE_ID_DEFAULT (1)

static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

typedef struct {
    uint32_t msg_no;    /**< message number */
    uint8_t hp;         /**< hp of node */
    uint8_t rssi;       /**< rssi of node */
    uint8_t lqi;        /**< lqi of node */
    char parent[IPV6_CUSTOM_ADDR_STR_LEN];    /**< parent of node */
} msg_ping_t;

static void ipv6_to_identifier(ipv6_addr_t *address, char *address_string)
{
    snprintf(address_string, IPV6_CUSTOM_ADDR_STR_LEN, "%02X%02X", address->u8[14], address->u8[15]);
//    ipv6_addr_to_str(address_string, address, IPV6_ADDR_MAX_STR_LEN);
}

static void _print_addr(void) 
{
    /* print all IPv6 addresses */
    printf("{\"IPv6 addresses\": [\"");
    netifs_print_ipv6("\", \"");
    puts("\"]}");
}

static int get_parent(char* parent)
{
    gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(1);
    if (!inst || !inst->dodag.parents) {
        return - 1;
    }
    ipv6_addr_t dodag_id = inst->dodag.parents->addr;

    ipv6_to_identifier(&dodag_id, parent);
    return 0;
}

static int get_stats(msg_ping_t *ping)
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
        ping->rssi = 0;
        ping->lqi = 10;
        return 0;
    }

    netstats_nb_t nb_stats;
    netstats_nb_get(&iface->netif, nce.l2addr, nce.l2addr_len, &nb_stats);
    ping->rssi = nb_stats.rssi;
    ping->lqi = nb_stats.lqi;
    return 0;
}

/* ================= Main ================= */
int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    printf("\n=== RPL Node Starting ===\n");
    ztimer_sleep(ZTIMER_SEC, 2);

    /* 1. Get first network interface */
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    if (!netif) {
        puts("No network interface found");
        return 1;
    }
    printf("Network interface PID: %d\n", netif->pid);

    /* 2. initialize RPL (auto init is included in Makefile) */
    gnrc_rpl_init(netif->pid);
    while (true) {
        printf("Node: waiting for parent\n");
        gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(INSTANCE_ID_DEFAULT);
        if (inst && inst->dodag.parents) {
            break;
        }
        ztimer_sleep(ZTIMER_SEC, 2);
    }
    char my_parent[5];
    get_parent(my_parent);
    printf("Found my parent: %s\n", my_parent);

    ztimer_sleep(ZTIMER_SEC, 30);

    _print_addr();

    /* Non-root node: send UDP packets to root */
    sock_udp_ep_t remote = { 0 };
    if (sock_udp_str2ep(&remote, "[2001:db8::1\%6]:1234") < 0) {
        puts("Unable to parse destination address");
        return 1;
    }

    uint32_t msg_no = 1;
    msg_ping_t ping = {0};
    while (msg_no < NUM_OF_PINGS) {
        if (get_parent(ping.parent) < 0) {
            printf("Lost my parent!\n");
            ztimer_sleep(ZTIMER_MSEC, 100);
            continue;
        }
        get_stats(&ping);
        ping.msg_no = msg_no;
        printf("Sending msg:%ld\n", msg_no);
        int res;
        /* send UDP msg */
        if((res = sock_udp_send(NULL, &ping, sizeof(ping), &remote)) < 0) {
            puts("could not send");
        }
        printf("Sent msg:%d\n", res);

        msg_no++;
        ztimer_sleep(ZTIMER_SEC, 1);
    }
    return 0;
}