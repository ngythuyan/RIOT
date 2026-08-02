/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Stuff
 *
 * @author      Thuy An Nguyen
 *
 * @}
 */
#include "structures.h"

bool cmp_addrs(uint8_t slot_address[], uint8_t rcv_address[])
{
    return memcmp(rcv_address, slot_address, IPV6_U8_ADDR_LEN) == 0;
}

uint8_t put_node(ipv6_addr_t addr, msg_ping_t *ping, node_info nodes[], uint8_t num)
{
    if (num >= NODE_MAP_SIZE) {
        puts("Structures: Array full!");
        return num;
    }

    /* find index of node */
    for(int i = 0; i < num; i++) {
        if(nodes[i].occupied && cmp_addrs(nodes[i].address, addr.u8)) {
            nodes[i].replies = ping->replies;
            nodes[i].sent = ping->msg_no;
            if (strncmp(nodes[i].current_parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN) != 0) {
                nodes[i].parent_changed++;
                if( nodes[i].parent_changed < MAX_PARENT_CHANGE) {
                    strncpy(nodes[i].parents[nodes[i].parent_changed], ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1); 
                    nodes[i].parents[nodes[i].parent_changed][IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
                }
                strncpy(nodes[i].current_parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1);
                nodes[i].current_parent[IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
            }
            return num;
        }
    }

    /* register new node */
    memcpy(nodes[num].address, addr.u8, IPV6_U8_ADDR_LEN);
    nodes[num].occupied = true;
    nodes[num].replies = ping->replies;
    nodes[num].sent = ping->msg_no;
    nodes[num].parent_changed = 0;
    ipv6_to_identifier(&addr, nodes[num].name);
    strncpy(nodes[num].current_parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1);
    nodes[num].current_parent[IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
    strncpy(nodes[num].parents[0], ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1); 
    nodes[num].parents[0][IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
    return num + 1;
}

void print_tree(node_info *info)
{
    (void) delay_us;
    printf("CURRENT-TREE-");
    for(int i = 0; i < NODE_MAP_SIZE; i++)
    {
        node_info node = info[i];
        if (node.occupied)
        {
            printf("-%s,%s", info[i].current_parent, info[i].name);
        }
    }
    printf("\n");
}

uint32_t running_avg(uint32_t current_average, uint32_t k, uint32_t new_value)
{
    return current_average + ((new_value - current_average) / (k + 1));
}
