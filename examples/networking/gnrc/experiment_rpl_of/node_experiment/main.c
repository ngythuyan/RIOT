/*
 * SPDX-FileCopyrightText: 2015 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Child node of experiment comparing RPL OFs
 *
 * @author      Thuy An Nguyen
 *
 * @}
 */

#include "../experiment.h"

#include "macros/utils.h"
#include "net/utils.h"
#include "mutex.h"

// for neighbor stats
#include "net/netstats.h"
#include "net/netstats/neighbor.h"

// for battery reading
#if IS_USED(MODULE_GNRC_RPL_MRHOF_ENERGY)
    #include "battery.h"
#endif

#ifndef RECORD_CACHE_SIZE
#  define RECORD_CACHE_SIZE (15)
#endif

static uint32_t seq_no = 1;
static sock_udp_t sock;

static char send_thread_stack[THREAD_STACKSIZE_MAIN + THREAD_EXTRA_STACKSIZE_PRINTF];
static char listen_thread_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];

static uint8_t buf_tx[PACKET_SIZE];
static msg_ping_t *ping = (void *)buf_tx;
static uint32_t initial_time;

static volatile bool running;
static sema_inv_t thread_sync;
static mutex_t mu = MUTEX_INIT;


/**
 * @brief   Recordings of time sent of messages
 * oldest message at first place
 */
struct {
    uint32_t msg_no;
    uint32_t time_tx_us;
} record_tx[RECORD_CACHE_SIZE];

static void _print_addr(void) 
{
    /* print all IPv6 addresses */
    printf("{\"IPv6 addresses\": [\"");
    netifs_print_ipv6("\", \"");
    puts("\"]}");
}

static uint32_t _get_rtt(uint32_t seq)
{
    uint32_t idx = seq % RECORD_CACHE_SIZE;
    if (record_tx[idx].msg_no == seq) {
        return ztimer_now(ZTIMER_USEC) - record_tx[idx].time_tx_us;
    }

    return 0;
}

static void _put_rtt(uint32_t seq)
{
    uint32_t now = ztimer_now(ZTIMER_USEC);
    uint32_t idx = seq % RECORD_CACHE_SIZE;

    record_tx[idx].msg_no = seq;
    record_tx[idx].time_tx_us = now;
}

static int get_parent(char* parent)
{
    unsigned state = irq_disable();
    gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(INSTANCE_ID_DEFAULT);
    if (!inst || !inst->dodag.parents) {
        irq_restore(state);
        return - 1;
    }
    ipv6_addr_t dodag_id = inst->dodag.parents->addr;
    irq_restore(state);

    ipv6_to_identifier(&dodag_id, parent);
    return 0;
}

static int get_stats(void)
{
    unsigned state = irq_disable();
    gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(INSTANCE_ID_DEFAULT);
    if (!inst || !inst->dodag.parents) {
        irq_restore(state);
        return - 1;
    }
    gnrc_rpl_dodag_t *dodag = &inst->dodag;
    uint16_t rank = dodag->my_rank;
    ipv6_addr_t parent_addr = dodag->parents->addr;
    kernel_pid_t iface_pid = dodag->iface;
    irq_restore(state);

    /* Hop Count*/
    ping->hp = rank / CONFIG_GNRC_RPL_DEFAULT_MIN_HOP_RANK_INCREASE;
    
    /* ETX and RSSI */
    gnrc_ipv6_nib_nc_t nce;
    gnrc_netif_t *iface = gnrc_netif_get_by_pid(iface_pid);
    if (gnrc_ipv6_nib_get_next_hop_l2addr(&parent_addr, iface, NULL, &nce)) {
        printf("Node: stats not found\n");
        ping->etx = 0;
        ping->rssi = 0;
        return 0;
    }

    netstats_nb_t nb_stats;
    netstats_nb_get(&iface->netif, nce.l2addr, nce.l2addr_len, &nb_stats);

    printf("Node: %ld -- ETX-%d, RSSI-%d\n", seq_no, nb_stats.etx, nb_stats.rssi);
    ping->etx = nb_stats.etx;
    ping->rssi = nb_stats.rssi;
    return 0;
}

/**
 * listen thread
 * @brief receive pong messages from server and calculate RTT
 */
static void *_listen_thread(void *ctx)
{
    (void)ctx;
    puts("Listen: listen thread start");

    static uint8_t buf[PACKET_SIZE];
    msg_pong_t *pong = (void *)buf;

    uint32_t timeout = (NUM_OF_PINGS * delay_us) +  (45 * US_PER_SEC);
    while (running) {
        /* receive pong */
        int res = sock_udp_recv(&sock, buf, PACKET_SIZE, timeout, NULL);

        if (res == -ETIMEDOUT) {
            puts("Listen: listen thread terminates");
            sema_inv_post(&thread_sync);
            return NULL;
        }
        else if (res < 0) {
            printf("Listen: Error while receiving: %d\n", res);
            continue;
        }
        else if (res == 0) {
            puts("Listen: No data received");
            continue;
        }

        printf("Listen: pong received %ld\n", pong->msg_no);
        /* calculate RTT and save */
        mutex_lock(&mu);
        uint32_t rtt = _get_rtt(pong->msg_no);
        ping->rtt_last = rtt;
        ping->replies++;
        mutex_unlock(&mu);
    }

    puts("Listen: listen thread terminates");
    sema_inv_post(&thread_sync);

    return NULL;
}

/* sending thread sends ping messages to server */
static void *_send_thread(void *ctx)
{
    puts("Send: sending thread start");

    /* prepare udp endpoint*/
    sock_udp_ep_t remote = { 0 };
    if (sock_udp_str2ep(&remote, SERVER_DEFAULT) < 0) {
        puts("Send: Unable to parse destination address");
    }

    initial_time = ztimer_now(ZTIMER_MSEC);

    while (seq_no < NUM_OF_PINGS) {
        /* prepare ping message */
        mutex_lock(&mu);
        int res = get_parent(ping->parent);
        if(res < 0) {
            mutex_unlock(&mu);
            ztimer_sleep(ZTIMER_USEC, delay_us);
            continue;
        }
        res = get_stats();
        if (res < 0) {
            mutex_unlock(&mu);
            ztimer_sleep(ZTIMER_USEC, delay_us);
            continue;
        }
    #if IS_USED(MODULE_GNRC_RPL_MRHOF_ENERGY)
        ping->energy = get_voltage();
    #endif
        ping->msg_no = seq_no;
        _put_rtt(seq_no);
        seq_no++;
        ping->time_passed = ztimer_now(ZTIMER_MSEC) - initial_time;
        msg_ping_t local_ping = *ping;
        mutex_unlock(&mu);

        /* send UDP msg */
        if((res = sock_udp_send(&sock, &local_ping, PACKET_SIZE, &remote)) < 0) {
            puts("Send: could not send");
        }
        ztimer_sleep(ZTIMER_USEC, delay_us);
    }

    puts("Send: sending thread terminates");
    sema_inv_post(&thread_sync);
    return NULL;
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

    /* init RPL */
    gnrc_rpl_init(netif->pid);
    puts("Node: Wait for parent");
    while (true) {
        printf("Node: waiting for parent\n");
        gnrc_rpl_instance_t *inst = gnrc_rpl_instance_get(INSTANCE_ID_DEFAULT);
        if (inst && inst->dodag.parents) {
            break;
        }
        ztimer_sleep(ZTIMER_SEC, 5);
    }
    char my_parent[5];
    get_parent(my_parent);
    printf("Found my parent: %s\n", my_parent);

    /* UDP setup */
    sock_udp_ep_t local = { .family = AF_INET6,
                            .netif = SOCK_ADDR_ANY_NETIF,
                            .port = PORT_DEFAULT };
    sock_udp_ep_t remote = { 0 };
    remote.family = AF_INET6;
    remote.port = PORT_DEFAULT;

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Node: Error creating UDP sock");
        return 1;
    }

    puts("Node: RPL and UDP setup finished");
    _print_addr();

    sema_inv_init(&thread_sync, 2);
    /* ================= setup listen thread ================= */
    running = true;
    thread_create(listen_thread_stack, sizeof(listen_thread_stack),
                 THREAD_PRIORITY_MAIN - 2, 0,
                 _listen_thread, NULL, "UDP receiver");
    puts("Node: Created listening thread");

    /* ================= send messages ================= */
   thread_create(send_thread_stack, sizeof(send_thread_stack),
                 THREAD_PRIORITY_MAIN - 1, 0,
                 _send_thread, &remote, "UDP sender");
   puts("Node: Created sending thread");

   /* ================= End of experiment ================= */

   sema_inv_wait(&thread_sync);

   /* send last message until success*/
    if (sock_udp_str2ep(&remote, SERVER_DEFAULT) < 0) {
        puts("Send: Unable to parse destination address");
    }
    ping->rssi = 0;
    ping->etx = 0;
    for(int i = 0; i < 5; i++) {
        sock_udp_send(&sock, ping, PACKET_SIZE, &remote);
        ztimer_sleep(ZTIMER_SEC, 1);
    }
    printf("Sent last message: %ld-%ld\n", ping->msg_no, ping->replies);

   sock_udp_close(&sock);
   memset(send_thread_stack, 0, sizeof(send_thread_stack));
   memset(listen_thread_stack, 0, sizeof(listen_thread_stack));
   
   puts("Node: Finished sending all pings!");
   return 0;
}
