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

static uint32_t initial_time = 0;

// record data
static node_info nodes[NODE_MAP_SIZE] = {0};

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
        uint32_t time_now = ztimer_now(ZTIMER_USEC) - initial_time;
        // Name;time;rtt;etx;energy;hp;rssi
        printf("Dispatcher:%s;%ld;%ld;%d;%d;%d;%d\n", 
               address, time_now, ping->rtt_last, ping->etx, 
               ping->energy, ping->hp, ping->rssi);
        if (put_node(payload->address, ping, nodes) == 1)
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
    static char server_buffer[PACKET_SIZE];
    
    initial_time = ztimer_now(ZTIMER_USEC);
    while (running) {
        /* receive ping */
        int res = sock_udp_recv(&sock, server_buffer,
                                PACKET_SIZE, 
                                SOCK_NO_TIMEOUT,
                                &remote);
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

    puts("Listen: Listen thread terminates");
    running = false;
    msg_t stop_msg = { .type = MSG_STOP };
    msg_send(&stop_msg, dispatch_pid);
    sema_inv_post(&thread_sync);
    return NULL;
}

/* ================= Main ================= */
static int exp_cmd(int argc, char **argv)
{
    if (argc < 2) {
        uint8_t node_amount = 0;
        uint8_t avg_parent_change = 0;
        uint8_t avg_pdr = 0;
        uint32_t avg_packets = 0;
        uint64_t total_packets = 0;

        for (int i = 0; i < NODE_MAP_SIZE; i++)
        {
            if(nodes[i].occupied)
            {
                // print Node infos
                uint8_t pdr = 0;
                if (nodes[i].sent > 0) 
                {
                    pdr = (nodes[i].replies / nodes[i].sent) * 100;
                }
                // Name;Parent;parent_changed;pdr;average_rtt;avg_etx;avg_rssi,avg_hp;average_energy
                printf("%s;%s;%d;%d;%ld;%d;%d;%d;%d\n", nodes[i].current_parent.child, 
                        nodes[i].current_parent.parent, nodes[i].parent_changed, pdr, nodes[i].avg_rtt, 
                        nodes[i].avg_etx, nodes[i].avg_rssi, nodes[i].avg_hp, nodes[i].avg_energy);
                // add average values
                avg_parent_change = running_avg(avg_parent_change, node_amount, nodes[i].parent_changed);
                avg_pdr = running_avg(avg_pdr, node_amount, pdr);
                avg_packets = running_avg(avg_packets, node_amount, nodes[i].sent);
                total_packets = total_packets + nodes[i].sent;
                node_amount++;
            }
        }
        printf("Overall data: \n Nodes in total: %d \nAverage number of parent changes: %d\n Average PDR: %d\n", 
                node_amount, avg_parent_change, avg_pdr);
        printf("In total %lld messages received by server, each node about %ld\n", total_packets, avg_packets);
        print_tree(nodes);
    }

    if (strcmp(argv[1], "stop") == 0) {
        running = false;
        sema_inv_init(&thread_sync, 2);
        sema_inv_wait(&thread_sync);
        sock_udp_close(&sock);
        memset(dispatch_thread_stack, 0, sizeof(dispatch_thread_stack));
        memset(listen_thread_stack, 0, sizeof(listen_thread_stack));
    }
    else if (strcmp(argv[1], "server") == 0) {
    }
    else {
        puts("error: invalid command");
    }
    return 0;
}

static const shell_command_t shell_commands[] = {
    {"experiment", "RPL OF experiment", exp_cmd},
    {NULL, NULL, NULL}
};

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
    
    /* ================= start shell ================= */
    puts("Root: Running Shell");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    
    return 0;
}
