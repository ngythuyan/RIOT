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

uint8_t put_node(ipv6_addr_t addr, msg_ping_t *ping, node_info nodes[], uint8_t nodes_registered)
{
    if (nodes_registered >= NODE_MAP_SIZE) {
        puts("structures: all node spots occupied!");
        return 0xFF;
    }

    /* find index of node */
    for (int i = 0; i < nodes_registered; i++) {
        if (nodes[i].occupied && (cmp_addrs(nodes[i].address, addr.u8))) {
            uint32_t new_msg_no = ping->msg_no;
            if (nodes[i].sent + 1 != new_msg_no) {
                nodes[i].missing++;
            }
            nodes[i].sent = ping->msg_no;
            if (strncmp(nodes[i].current_parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN) != 0) {
                nodes[i].parent_changed++;
                if (nodes[i].parent_changed < MAX_PARENT_CHANGE) {
                    strncpy(nodes[i].parents[nodes[i].parent_changed], ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1); 
                    nodes[i].parents[nodes[i].parent_changed][IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
                }
                strncpy(nodes[i].current_parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1);
                nodes[i].current_parent[IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
                print_tree(nodes);
            }
            return nodes_registered;
        }
    }

    /* register new node */
    nodes[nodes_registered].occupied = true;
    memcpy(nodes[nodes_registered].address, addr.u8, IPV6_U8_ADDR_LEN);
    ipv6_to_identifier(&addr, nodes[nodes_registered].name);
    nodes[nodes_registered].sent = ping->msg_no;
    nodes[nodes_registered].parent_changed = 0;
    strncpy(nodes[nodes_registered].parents[nodes[nodes_registered].parent_changed], ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1); 
    nodes[nodes_registered].parents[nodes[nodes_registered].parent_changed][IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
    strncpy(nodes[nodes_registered].current_parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1);
    nodes[nodes_registered].current_parent[IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
    print_tree(nodes);
    return nodes_registered + 1;
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
            printf("-%s,%s", node.current_parent, node.name);
        }
    }
    printf("\n");
}

uint32_t running_avg(uint32_t current_average, uint32_t k, uint32_t new_value)
{
    return current_average + ((new_value - current_average) / (k + 1));
}
