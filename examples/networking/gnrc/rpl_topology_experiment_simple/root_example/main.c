#include <stdio.h>
#include <string.h>
#include "ztimer.h"
#include "thread.h"

#include "msg.h"
#include "net/sock/udp.h"
#include "net/sock/util.h"
#include "net/sock/async/event.h"
#include "net/ipv6/addr.h"

#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/rpl.h"
#include "net/gnrc/rpl/dodag.h"
#include "net/gnrc/udp.h"

#define IPV6_CUSTOM_ADDR_STR_LEN (5)
#define MAX_NODES (100)

typedef struct node {
    uint8_t here;
    char parent[5];
    char child[5]; /* name of node */
    uint32_t parent_change;
} node_t;

#define MAIN_QUEUE_SIZE (100)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
static uint32_t time_start;
static node_t nodes[MAX_NODES];
static sock_udp_ep_t remote;

static void _print_addr(void) 
{
    /* print all IPv6 addresses */
    printf("{\"IPv6 addresses\": [\"");
    netifs_print_ipv6("\", \"");
    puts("\"]}");
}

static void ipv6_to_identifier(ipv6_addr_t *address, char *address_string)
{
    snprintf(address_string, IPV6_CUSTOM_ADDR_STR_LEN, "%02X%02X", address->u8[14], address->u8[15]);
//    ipv6_addr_to_str(address_string, address, IPV6_ADDR_MAX_STR_LEN);
}

static void print_tree(node_t *nodes) {
    printf("Tree:");
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].here > 0) {
            printf(";%s-%s", nodes[i].parent, nodes[i].child);
        }
    }
    printf("\n");
}
/* ================= Main ================= */
int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    printf("\n=== RPL Node Starting ===\n");
    _print_addr();
    ztimer_sleep(ZTIMER_SEC, 2);
    time_start = ztimer_now(ZTIMER_SEC);

    /* Get first network interface */
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    if (!netif) {
        puts("No network interface found");
        return 1;
    }
    printf("Network interface PID: %d\n", netif->pid);

    // TASK 1 initialize RPL (auto init is included in Makefile)
    gnrc_rpl_init(netif->pid);
    ztimer_sleep(ZTIMER_SEC, 2);

    printf("=== Initializing as DODAG Root ===\n");

    ipv6_addr_t dodag_id;
	// Create DODAG Root with ip and initialize Root Node

    /* check if node already has global IPv6 address, if not add one */
    ipv6_addr_t addrs[2];
    ssize_t get_ipv6 = netifs_get_ipv6(addrs, 2);

    if(get_ipv6 < 2) {
        ipv6_addr_from_str(&dodag_id, "2001:db8::1");
        gnrc_netif_ipv6_addr_add(netif, &dodag_id, 64, 0);   
    }
    else
    {
        dodag_id = addrs[1];
    }

    _print_addr();

    /* init RPL root */
    gnrc_rpl_instance_t *inst = gnrc_rpl_root_init(1, &dodag_id, false, false);
    if (!inst) {
        puts("RPL Root init failed");
        return 1;
    }

    /* UDP setup */
    sock_udp_t sock;
    sock_udp_ep_t server = { .port = atoi("1234"), .family = AF_INET6 };
    if(sock_udp_create(&sock, &server, NULL, 0) < 0) {
        return 1;
    }
    remote.family = AF_INET6;
    remote.port = 1234;

    uint32_t time_start = ztimer_now(ZTIMER_MSEC);
    static char server_buffer[128];
    int num = 0;
    
    while (1) {
        /* receive UDP */
        int res;

        if ((res = sock_udp_recv(&sock, server_buffer,
                                sizeof(server_buffer), SOCK_NO_TIMEOUT,
                                &remote)) < 0) {
            printf("Error while receiving: %d\n", res);
            continue;
        }
        else if (res == 0) {
            puts("No data received");
            continue;
        }
        char parent[5];
        strncpy(parent, server_buffer, 5);
        char sender[5];
        ipv6_to_identifier((ipv6_addr_t *)&remote.addr.ipv6, sender);
        bool found = false;

        /* find index of node */
        for (int i = 0; i < num; i++) {
            if (nodes[i].here > 0 && strncmp(nodes[i].child, sender, 5) == 0) {
                // Update existing entry
                int cmp = strncmp(nodes[i].parent, parent, 5);
                if (cmp != 0) {
                    nodes[i].parent_change++;
                    strncpy(nodes[i].parent, parent, 5);
                    nodes[i].parent[4] = '\0';
                    print_tree(nodes);
                    printf("Time:%ld-NUM:%d-NEW:%s\n", ztimer_now(ZTIMER_MSEC) - time_start, num, sender);
                }
                found = true;
                break;
            }
        }
        if(found) {
            continue;
        }

        /* register new node */
        nodes[num].here = 1;
        nodes[num].parent_change = 0;
        strncpy(nodes[num].child, sender, 5);
        strncpy(nodes[num].parent, parent, 5);
        num++;
        print_tree(nodes);
        printf("Time:%ld-NUM:%d-NEW:%s\n", ztimer_now(ZTIMER_MSEC) - time_start, num, sender);
    }
    return 0;
}
