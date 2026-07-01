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

typedef struct __attribute__((packed)) {
    int32_t energetic_happiness;
    int32_t voltage;
} energy_t;

#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
static uint32_t time_start;

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
    _print_addr();
    ztimer_sleep(ZTIMER_SEC, 2);
    time_start = ztimer_now(ZTIMER_USEC);

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

    static char server_buffer[MAIN_QUEUE_SIZE + sizeof(energy_t)];
    energy_t* e = (void*) server_buffer;
    
    while (1) {
        /* receive UDP */
        puts("Waiting for packet...");
        int res;

        if ((res = sock_udp_recv(&sock, server_buffer,
                                sizeof(server_buffer) - 1, SOCK_NO_TIMEOUT,
                                NULL)) < 0) {
            printf("Error while receiving: %d\n", res);
        }
        else if (res == 0) {
            puts("No data received");
        }
        else {
            uint32_t time_now = ztimer_now(ZTIMER_USEC) - time_start;
            printf("Received:%ld;%ld;%ld\n", time_now, e->energetic_happiness, e->voltage);
        }
    }
    return 0;
}
