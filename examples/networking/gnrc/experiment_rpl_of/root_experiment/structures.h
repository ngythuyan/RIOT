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
#define NODE_MAP_SIZE  (NUM_OF_NODES + 10)
#endif

#ifndef MAX_PARENT_CHANGE
#define MAX_PARENT_CHANGE  (15)
#endif

#ifndef IPV6_U8_ADDR_LEN
#define IPV6_U8_ADDR_LEN  (16)
#endif

typedef struct {
    uint8_t address[IPV6_U8_ADDR_LEN];
    uint32_t replies;
    uint32_t sent;
    uint8_t parent_changed;
    char parents[MAX_PARENT_CHANGE][IPV6_CUSTOM_ADDR_STR_LEN];
    bool occupied;
    char name[IPV6_CUSTOM_ADDR_STR_LEN];
    char current_parent[IPV6_CUSTOM_ADDR_STR_LEN];
} node_info;

/**
 * @brief   checks if two provided addresses are the same
 * @return true when the same, false when different
 */
bool cmp_addrs(uint8_t slot_address[], uint8_t rcv_address[]);

/**
 * @brief   saves node info, creates a new node when node not saved yet
 * @return 0 when everything okay, 1 when tree was updated
 */
uint8_t put_node(ipv6_addr_t addr, msg_ping_t *ping, node_info nodes[], uint8_t num);

/**
 * @brief   prints current topology of network
 */
void print_tree(node_info *info);

/**
 * @brief   calculates running average
 */
uint32_t running_avg(uint32_t current_average, uint32_t k, uint32_t new_value);