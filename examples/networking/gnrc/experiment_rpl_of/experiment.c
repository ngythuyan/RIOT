#include "experiment.h"

void ipv6_to_identifier(ipv6_addr_t *address, char *address_string)
{
    (void) delay_us;
    snprintf(address_string, IPV6_CUSTOM_ADDR_STR_LEN, "%02X%02X", address->u8[14], address->u8[15]);
//    ipv6_addr_to_str(address_string, address, IPV6_ADDR_MAX_STR_LEN);
}