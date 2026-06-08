/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       RPL OF experiment structs and configurations
 *
 * @author      Thuy An Nguyen
 *
 * @}
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "ztimer.h"
#include "thread.h"

#include "msg.h"
#include "net/sock/udp.h"
#include "net/sock/util.h"
#include "net/sock/async/event.h"
#include "net/ipv6/addr.h"

#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/rpl.h"
#include "net/gnrc/rpl/dodag.h"
#include "net/gnrc/udp.h"

#include "sema_inv.h"

// not sure if I actually need these
#include "net/gnrc/netif/hdr.h"
#include "net/ipv6/addr.h"
#include "net/netif.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/pktdump.h"

/* ================= Parameter configuration ================= */
/* load of experiment */
static uint32_t delay_us = US_PER_SEC;
//static uint32_t delay_us = 500 * US_PER_MS;
//static uint32_t delay_us = 50 * US_PER_MS;

/* duration of experiment */
#ifndef NUM_OF_PINGS
#define NUM_OF_PINGS  (1100)
#endif

/* Size of network */
#ifndef NUM_OF_NODES
#define NUM_OF_NODES  (20)
#endif

/* Packet Size */
#ifndef PACKET_SIZE
#define PACKET_SIZE  (128)
#endif
/* ================= Configuration END ================= */

/**
 * @brief   Length of String ipv6_to_identifier returns (including Null termination)
 */
#ifndef IPV6_CUSTOM_ADDR_STR_LEN
#define IPV6_CUSTOM_ADDR_STR_LEN  (5)
//#define IPV6_ADDR_STR_LEN  (IPV6_ADDR_MAX_STR_LEN)
#endif

/**
 * @brief   Maximum size of a benchmark packet
 */
#ifndef INSTANCE_ID_DEFAULT
#define INSTANCE_ID_DEFAULT  (1)
#endif

/**
 * @brief   Maximum size of a benchmark packet
 */
#ifndef PAYLOAD_SIZE_MAX
#define PAYLOAD_SIZE_MAX  (128)
#endif

/**
 * @brief   Default address of server
 */
#ifndef ADDRESS_DEFAULT
#define ADDRESS_DEFAULT    "2001:db8::1"
#endif

/**
 * @brief   Default address, iface, port of server
 */
#ifndef SERVER_DEFAULT
#define SERVER_DEFAULT    "[2001:db8::1\%7]:12345"
#endif

/**
 * @brief   Default port 
 */
#ifndef PORT_DEFAULT
#define PORT_DEFAULT      (12345)
#endif

/**
 * @brief   Ping message to the server
 * @note    Both server and client are assumed to be little endian machines
 * @{
 */
typedef struct {
    uint32_t msg_no;    /**< message number */
    uint32_t replies;   /**< number of replies received from server    */
    uint32_t rtt_last;  /**< round trip time of the last packet        */
    char parent[IPV6_CUSTOM_ADDR_STR_LEN];    /**< parent of node */
    uint16_t etx;        /**< variable length payload */
    uint16_t energy;     /**< variable length payload */
    uint8_t hp;         /**< variable length payload */
    uint8_t rssi;       /**< variable length payload */
} msg_ping_t;
/** @} */

/**
 * @brief   Pong message from the server
 * @note    Both server and client are assumed to be little endian machines
 * @{
 */
typedef struct {
    uint32_t msg_no;    /**< message number */
} msg_pong_t;
/** @} */

void ipv6_to_identifier(ipv6_addr_t *address, char *address_string);

typedef struct {
    msg_ping_t ping;
    ipv6_addr_t address;
} dispatch_payload_t;