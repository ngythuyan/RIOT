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
#include "xtimer.h"
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

#include "board.h"
#include "saul_reg.h"
#include "ws281x.h"
#include "ws281x_params.h"

#include "mrhof_energy_test.h"

#define RPL_PREFIX              (0xfdeadbeef0000000)
#define RPL_IID                 (0x1)
#define RPL_PREFIX_LEN          (64U)
#define PORT_ALL             (8888)
#define MAIN_QUEUE_SIZE     (8)

#define MAIN_QUEUE_SIZE (8)

typedef struct __attribute__((packed)) {
    int32_t energetic_happiness;
    int32_t voltage;
} energy_t;

static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
static energy_t e1;

static void _print_addr(void) 
{
    /* print all IPv6 addresses */
    printf("{\"IPv6 addresses\": [\"");
    netifs_print_ipv6("\", \"");
    puts("\"]}");
}

/* ================= Main ================= */
int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    printf("\n=== RPL Node Starting ===\n");
    xtimer_sleep(2);

    ws281x_t dev;
    init_mrhof_energy_test();

    /* 1. Get first network interface */
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    if (!netif) {
        puts("No network interface found");
        return 1;
    }
    printf("Network interface PID: %d\n", netif->pid);

    /* 2. initialize RPL (auto init is included in Makefile) */
    gnrc_rpl_init(netif->pid);
    xtimer_sleep(2);

    _print_addr();

    /* Non-root node: send UDP packets to root */
    sock_udp_ep_t remote = { 0 };
    if (sock_udp_str2ep(&remote, "[2001:db8::1\%7]:1234") < 0) {
        puts("Unable to parse destination address");
        return 1;
    }
    puts("Turn LED on");
    LED0_ON;
    LED1_ON;
    ws281x_init (&dev, &ws281x_params[0]);
    color_rgb_t color = {255, 255, 255};
    ws281x_set (&dev, 0, color);
    ws281x_write (&dev);
    puts("LED on");
    while (1) {
        // read voltage
        e1.energetic_happiness = get_energetic_happiness_test();
        e1.voltage = get_remaining_energy_test();
        printf("V=%ld\nE_E=%ld\n", e1.voltage, e1.energetic_happiness);

        int res;
        /* send UDP msg */
        if((res = sock_udp_send(NULL, &e1, sizeof(e1), &remote)) < 0) {
            puts("could not send");
        }
        else {
            printf("Success: send %u byte to server\n", (unsigned) res);
        }

        xtimer_sleep(5);
    }
    return 0;
}