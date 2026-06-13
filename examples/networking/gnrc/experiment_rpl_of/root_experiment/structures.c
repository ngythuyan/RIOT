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

uint32_t hash_ipv6(uint8_t addr[])
{
    uint32_t hash = 0;
    for(int i = 0; i < 16; i++)
    {
        hash = hash * 7 + addr[i];
    }
    hash = hash % NODE_MAP_SIZE;
    return hash;
}

bool cmp_addrs(uint8_t slot_address[], uint8_t rcv_address[])
{
    return memcmp(rcv_address, slot_address, IPV6_U8_ADDR_LEN) == 0;
}

uint8_t put_node(ipv6_addr_t addr, msg_ping_t *ping, node_info nodes[])
{
    /* find free index */
    uint32_t hash = hash_ipv6(addr.u8);
    node_info *target_slot = &nodes[hash];
    bool check = cmp_addrs(target_slot->address, addr.u8);
    while(target_slot->occupied && !check) 
    {
        hash++;
        target_slot = &nodes[hash];
        check = cmp_addrs(target_slot->address, addr.u8);
        if(hash >= NODE_MAP_SIZE)
        {
            hash = 0;
        }
    }

    /* put in data */
    memcpy(target_slot->address, addr.u8, IPV6_U8_ADDR_LEN);
    target_slot->replies = ping->replies;
    target_slot->sent = ping->msg_no;
    target_slot->avg_rtt = running_avg(target_slot->avg_rtt, ping->replies, ping->rtt_last);
    target_slot->avg_etx = running_avg(target_slot->avg_etx, ping->replies, ping->etx);
    target_slot->avg_hp = running_avg(target_slot->avg_hp, ping->replies, ping->hp);
    target_slot->avg_rssi = running_avg(target_slot->avg_rssi, ping->replies, ping->rssi);
    target_slot->avg_energy = running_avg(target_slot->avg_energy, ping->replies, ping->energy);

    /* save information parent child relationship */
    if(target_slot->occupied == false)
    {
        target_slot->occupied = true;

        ipv6_to_identifier(&addr, target_slot->current_parent.child);
        strncpy(target_slot->current_parent.parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1);
        target_slot->current_parent.parent[IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
        return 1;
    }
    else if (strncmp(target_slot->current_parent.parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN) != 0)
    {
        target_slot->parent_changed++;
        strncpy(target_slot->current_parent.parent, ping->parent, IPV6_CUSTOM_ADDR_STR_LEN - 1);
        target_slot->current_parent.parent[IPV6_CUSTOM_ADDR_STR_LEN - 1] = '\0';
        return 1;
    }
    return 0;
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
            printf("-%s:%s", node.current_parent.parent, node.current_parent.child);
        }
    }
    printf("\n");
}

uint32_t running_avg(uint32_t current_average, uint32_t k, uint32_t new_value)
{
    return current_average + ((new_value - current_average) / (k + 1));
}
