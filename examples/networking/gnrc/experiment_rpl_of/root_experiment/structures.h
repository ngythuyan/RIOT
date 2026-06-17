/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hash table like structure
 *
 * @author      Thuy An Nguyen
 *
 * @}
 */
#pragma once

#include "../experiment.h"

#ifndef NODE_MAP_SIZE
#define NODE_MAP_SIZE  (NUM_OF_NODES * 3 / 2)
#endif

#ifndef MAX_PARENT_CHANGE
#define MAX_PARENT_CHANGE  (5)
#endif

#ifndef IPV6_U8_ADDR_LEN
#define IPV6_U8_ADDR_LEN  (16)
#endif

typedef struct
{
    char child[IPV6_CUSTOM_ADDR_STR_LEN];
    char parent[IPV6_CUSTOM_ADDR_STR_LEN];
} edge;

typedef struct {
    uint8_t address[IPV6_U8_ADDR_LEN];
    uint32_t replies;
    uint32_t sent;
    uint8_t parent_changed;
    bool occupied;
    edge current_parent;
} node_info;

/**
 * @brief   checks if two provided addresses are the same
 * @return true when the same, false when different
 */
bool cmp_addrs(uint8_t slot_address[], uint8_t rcv_address[]);

/**
 * @brief   Gets hash of IPv6 address
 */
uint32_t hash_ipv6(uint8_t addr[]);

/**
 * @brief   saves node info, creates a new node when node not saved yet
 * @return 0 when everything okay, 1 when tree was updated
 */
uint8_t put_node(ipv6_addr_t addr, msg_ping_t *ping, node_info nodes[]);

/**
 * @brief   prints current topology of network
 */
void print_tree(node_info *info);

/**
 * @brief   calculates running average
 */
uint32_t running_avg(uint32_t current_average, uint32_t k, uint32_t new_value);