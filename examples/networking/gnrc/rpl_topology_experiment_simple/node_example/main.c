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
#define NUM_OF_PINGS (1000)

#define MAIN_QUEUE_SIZE 32
#define IPV6_CUSTOM_ADDR_STR_LEN (5)
#define INSTANCE_ID_DEFAULT (1)

static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

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
    while (msg_no < NUM_OF_PINGS) {
        char parent[5];
        if (get_parent(parent) < 0) {
            printf("Lost my parent!\n");
            ztimer_sleep(ZTIMER_MSEC, 100);
            continue;
        }
        
        printf("Sending msg:%ld\n", msg_no);
        int res;
        /* send UDP msg */
        if((res = sock_udp_send(NULL, parent, sizeof(parent), &remote)) < 0) {
            puts("could not send");
        }
        printf("Sent msg:%d\n", res);

        msg_no++;
        ztimer_sleep(ZTIMER_SEC, 1);
    }
    return 0;
}