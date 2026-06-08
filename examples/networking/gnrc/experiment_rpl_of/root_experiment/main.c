/*
 * SPDX-FileCopyrightText: 2015 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Root node of experiment comparing RPL OFs
 *
 * @author      Thuy An Nguyen
 *
 * @}
 */

#include "../experiment.h"
#include "shell.h"
#include "structures.h"

#define DISPATCH_QUEUE_SIZE (10)
#define MSG_STOP 0xFF

static sock_udp_t sock;
static sock_udp_ep_t remote;

static char listen_thread_stack[THREAD_STACKSIZE_DEFAULT];
static char dispatch_thread_stack[THREAD_STACKSIZE_DEFAULT];
static msg_t dispatch_queue[DISPATCH_QUEUE_SIZE];
static kernel_pid_t dispatch_pid;

static uint8_t buf_tx[PAYLOAD_SIZE_MAX + sizeof(msg_pong_t)];
static msg_pong_t *pong = (void *)buf_tx;

static sema_inv_t thread_sync;
static bool running;
static bool first_msg_rcvd = false;

// record data
static node_info nodes[NUM_OF_NODES];

static void _print_addr(void) 
{
    /* print all IPv6 addresses */
    printf("Root: {\"IPv6 addresses\": [\"");
    netifs_print_ipv6("\", \"");
    puts("\"]}");
}

static void *_dispatch_thread(void *arg)
{
    (void)arg;
    msg_init_queue(dispatch_queue, DISPATCH_QUEUE_SIZE);

    msg_t msg;
    while (running) {
        msg_receive(&msg);

        if (msg.type == MSG_STOP)
        {
            break;
        }
        dispatch_payload_t *payload = (dispatch_payload_t *)msg.content.ptr;
        msg_ping_t *ping = &payload->ping;
        char address[5] = {0};
        ipv6_to_identifier(&payload->address, address);
        printf("Dispatcher: %s;%ld;%ld;%d;%d;%d;%d\n", 
               address, ping->replies, ping->rtt_last, 
               ping->etx, ping->energy, ping->hp, ping->rssi);
        if (put_node(payload->address, ping->replies, ping->parent, nodes) == 1)
        {
            printf("Dispatcher: ");
            print_tree(nodes);
        }
        
        free(payload);
    }
    sema_inv_post(&thread_sync);
    return NULL;
}

static void *_listen_thread(void *ctx)
{
    (void)ctx;
//    uint32_t timeout = 3 * delay_us * US_PER_MS;
    static char server_buffer[PACKET_SIZE];
    
    while (running) {
        /* receive ping */
        int res = sock_udp_recv(&sock, server_buffer,
                                PACKET_SIZE, 
                                SOCK_NO_TIMEOUT,
                                &remote);
        // if (res == -ETIMEDOUT)
        // {
        //     if (first_msg_rcvd)
        //     {
        //         puts("Listen: no messages received. Listen thread terminates");
        //         running = false;
        //         msg_t stop_msg = { .type = MSG_STOP };
        //         msg_send(&stop_msg, dispatch_pid);
        //         sema_inv_post(&thread_sync);
        //         return NULL;
        //     }
        //     continue;
        // }
        // else if (res < 0) 
        if (res < 0) 
        {
            char address_string[5] = {0};
            ipv6_to_identifier((ipv6_addr_t *)&remote.addr.ipv6, address_string);
            printf("Listen: Error while receiving message from %*s\n", res, address_string);
            continue;
        }

        first_msg_rcvd = true;

        /* send pong back */
        msg_ping_t *ping = (void *)server_buffer;
        pong->msg_no = ping->msg_no;
        if (sock_udp_send(&sock, pong, PACKET_SIZE, &remote) < 0) {
            puts("Listen: Error sending reply");
        }

        /* Copy payload and hand off to dispatcher */
        dispatch_payload_t *copy = malloc(sizeof(dispatch_payload_t));
        if (copy == NULL) {
            puts("Listen: Lost data");
            continue;
        }
        memcpy(&copy->ping, server_buffer, sizeof(msg_ping_t));
        memcpy(&copy->address, &remote.addr.ipv6, sizeof(ipv6_addr_t));

        msg_t msg;
        msg.content.ptr = copy;
        if (msg_send(&msg, dispatch_pid) <= 0) {
            puts("Listen: Dispatch send failed");
            free(copy);
            continue;
        }
    }

    // Never reached
    puts("Listen: listen thread terminates");
    sema_inv_post(&thread_sync);
    return NULL;
}

/* ================= Main ================= */
int main(void)
{
    (void) delay_us;
    ztimer_sleep(ZTIMER_SEC, 2);

    /* ================= Init Root ================= */
    /* Get first network interface */
    gnrc_netif_t *netiface = gnrc_netif_iter(NULL);
    if (!netiface) {
        puts("Root: No network interface found");
        return 1;
    }
    printf("Root: Network interface PID: %d\n", netiface->pid);

    /* init RPL */
    gnrc_rpl_init(netiface->pid);
    ztimer_sleep(ZTIMER_SEC, 2);

    /* add IPv6 address */
    ipv6_addr_t dodag_id;
    ipv6_addr_from_str(&dodag_id, ADDRESS_DEFAULT);
    gnrc_netif_ipv6_addr_add(netiface, &dodag_id, 64, 0);

    _print_addr();

    /* init RPL root */
    gnrc_rpl_instance_t *inst = gnrc_rpl_root_init(1, &dodag_id, false, false);
    if (!inst) {
        puts("Root: RPL Root init failed");
        return 1;
    }

    /* UDP setup */
    sock_udp_ep_t local = { .family = AF_INET6,
                            .netif = SOCK_ADDR_ANY_NETIF,
                            .port = PORT_DEFAULT };
    remote.family = AF_INET6;
    remote.port = PORT_DEFAULT;

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Root: Error creating UDP sock");
        return 1;
    }
    
    /* ================= receive messages ================= */
    running = true;
    thread_create(listen_thread_stack, sizeof(listen_thread_stack),
                THREAD_PRIORITY_MAIN - 1, 0,
                _listen_thread, NULL, "UDP Listen Thread");
    puts("Root: Created listening thread");
    /* ================= handle messages ================= */
    dispatch_pid = thread_create(dispatch_thread_stack, sizeof(dispatch_thread_stack),
                THREAD_PRIORITY_MAIN - 2, 0,
                _dispatch_thread, NULL, "Dispatcher");
    puts("Root: Created DIspatcher thread");

    sema_inv_init(&thread_sync, 2);
    sema_inv_wait(&thread_sync);
    sock_udp_close(&sock);
    memset(dispatch_thread_stack, 0, sizeof(dispatch_thread_stack));
    memset(listen_thread_stack, 0, sizeof(listen_thread_stack));
    
    /* ================= start shell ================= */
    puts("Root: Running Shell");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    
    return 0;
}
