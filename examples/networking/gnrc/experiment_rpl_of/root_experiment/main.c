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

#define DISPATCH_QUEUE_SIZE (NUM_OF_NODES * 4)
#define MSG_STOP 0xFF

static sock_udp_t sock;
static sock_udp_ep_t remote;

static char listen_thread_stack[THREAD_STACKSIZE_MAIN + THREAD_EXTRA_STACKSIZE_PRINTF];
static char dispatch_thread_stack[THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF];
static msg_t dispatch_queue[DISPATCH_QUEUE_SIZE];
static kernel_pid_t dispatch_pid;

static uint8_t buf_tx[PAYLOAD_SIZE_MAX + sizeof(msg_pong_t)];
static msg_pong_t *pong = (void *)buf_tx;

static sema_inv_t thread_sync;
static bool running;
static bool first_msg_rcvd = false;

// record data
static node_info nodes[NODE_MAP_SIZE] = {0};

static void _print_addr(void) 
{
    /* print all IPv6 addresses */
    printf("Root: {\"IPv6 addresses\": [\"");
    netifs_print_ipv6("\", \"");
    puts("\"]}");
}

static void _print_data(void)
{
    uint8_t node_amount = 0;
    uint8_t total_parent_change = 0;
    printf("Summary:\n\n");
    for (int i = 0; i < NODE_MAP_SIZE; i++)
    {
        if(nodes[i].occupied)
        {
            // print Node infos
            // Name;Parent;parent_changed;sent;replies
            printf("Printer:%s;%s;%d;%ld;%ld\n", nodes[i].name,
                    nodes[i].current_parent, nodes[i].parent_changed,
                    nodes[i].sent, nodes[i].replies);
            printf("Parents:%s:", nodes[i].name);
            for(int j = 0; j < nodes[i].parent_changed; j++) {
                printf("%s;", nodes[i].parents[j]);
            }
            printf("\n");
            // add values
            total_parent_change = total_parent_change + nodes[i].parent_changed;
            node_amount++;
        }
    }
    printf("Total_parent_changes: %d\n", total_parent_change);
    printf("Total nodes: %d\n", node_amount);
    print_tree(nodes);
}

static void *_dispatch_thread(void *arg)
{
    (void)arg;
    msg_init_queue(dispatch_queue, DISPATCH_QUEUE_SIZE);
    bool send_finished = false;
    uint8_t node_no = 0;
    msg_t msg;
    while (!send_finished || msg_avail() > 0) {
        msg_receive(&msg);

        if (msg.type == MSG_STOP)
        {
            send_finished = true;
            continue;
        }

        dispatch_payload_t *payload = (dispatch_payload_t *)msg.content.ptr;
        msg_ping_t *ping = &payload->ping;
        char address[5] = {0};
        ipv6_to_identifier(&payload->address, address);

        if(ping->last_info == 1) {
            printf("Node_last_info:%s;%ld;%ld\n", address, ping->msg_no, ping->replies);
            free(payload);
            continue;
        }

        // Name;time;rtt;rtt_msg_no;etx;hp;rssi;lqi;replies;sent
        printf("Dispatcher:%s;%ld;%ld;%ld;%d;%d;%d;%d;%ld;%ld\n", 
               address, ping->time_passed, ping->rtt_last, ping->rtt_last_no, 
               ping->etx, ping->hp, ping->rssi, ping->lqi, ping->replies, 
               ping->msg_no);
        node_no = put_node(payload->address, ping, nodes, node_no);

        free(payload);
    }
    puts("Dispatch: Dispatch thread terminates");
    sema_inv_post(&thread_sync);
    _print_data();
    return NULL;
}

static void *_listen_thread(void *ctx)
{
    (void)ctx;
    static char server_buffer[PAYLOAD_SIZE_MAX];
    uint32_t timeout = (NUM_OF_PINGS * delay_us) + (120 * US_PER_SEC);
    
    while (running) {
        /* receive ping */
        int res = sock_udp_recv(&sock, server_buffer,
                                PAYLOAD_SIZE_MAX, 
                                timeout,
                                &remote);
        if (res == -ETIMEDOUT) {
            if (!first_msg_rcvd) {
                continue;
            }
            puts("Listen: Listen thread terminates");
            running = false;
            msg_t stop_msg = { .type = MSG_STOP };
            msg_send(&stop_msg, dispatch_pid);
            sema_inv_post(&thread_sync);
            return NULL;
        }
        else if (res < 0) 
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
        if (sock_udp_send(&sock, pong, sizeof(pong), &remote) < 0) {
            puts("Listen: Error sending reply");
        }
        else {
            if (ping->last_info == 0) {
                char sender[5];
                ipv6_to_identifier((ipv6_addr_t *)&remote.addr.ipv6, sender);
                printf("Listener:Sent_pong;%ld;%s\n", pong->msg_no, sender);
            }
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
            printf("Listen: Dispatch queue full\n");
            free(copy);
            continue;
        }
    }

    /* Never reached */
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
        _print_data();
    }

    if (strcmp(argv[1], "stop") == 0) {
        printf("Stopping experiment: %d\n", 3);
        running = false;
        sema_inv_init(&thread_sync, 2);
        sema_inv_wait(&thread_sync);
        sock_udp_close(&sock);
        memset(dispatch_thread_stack, 0, sizeof(dispatch_thread_stack));
        memset(listen_thread_stack, 0, sizeof(listen_thread_stack));
    }
    else if (strcmp(argv[1], "running") == 0) {
        if (running) {
            printf("Experiment still ongoing\n");
        }
        else {
            printf("Experiment stopped\n");
        }
    }
    else if(strcmp(argv[1], "tree") == 0) {
        print_tree(nodes);
    }
    else {
        puts("error: invalid command");
    }
    return 0;
}

static const shell_command_t shell_commands[] = {
    {"exp", "RPL OF experiment", exp_cmd},
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
    printf("\n");

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
