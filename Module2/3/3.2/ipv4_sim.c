#include "ipv4_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

ip_status_t parse_ip(const char *str, ipv4_addr_t *ip) {
    if (!str || !ip) return IP_ERR_INVALID_IP;
    struct in_addr addr;
    if (inet_pton(AF_INET, str, &addr) != 1) {
        return IP_ERR_INVALID_IP;
    }
    ip->value = ntohl(addr.s_addr);
    return IP_OK;
}

ip_status_t parse_mask(const char *str, ipv4_addr_t *mask) {
    if (!str || !mask) return IP_ERR_INVALID_MASK;
    ipv4_addr_t parsed_ip;
    if (parse_ip(str, &parsed_ip) != IP_OK) {
        return IP_ERR_INVALID_MASK;
    }
    
    uint32_t rev = ~parsed_ip.value;
    if ((rev & (rev + 1)) != 0) {
        return IP_ERR_INVALID_MASK;
    }
    
    mask->value = parsed_ip.value;
    return IP_OK;
}

int is_in_subnet(ipv4_addr_t ip, ipv4_addr_t gateway, ipv4_addr_t mask) {
    return (ip.value & mask.value) == (gateway.value & mask.value);
}

void ip_to_string(ipv4_addr_t ip, char *str) {
    if (!str) return;
    sprintf(str, "%u.%u.%u.%u",
            (ip.value >> 24) & 0xFF,
            (ip.value >> 16) & 0xFF,
            (ip.value >> 8) & 0xFF,
            ip.value & 0xFF);
}

ipv4_addr_t generate_random_ip(void) {
    ipv4_addr_t ip;
    uint32_t val = 0;
    val |= ((uint32_t)(rand() & 0xFF)) << 24;
    val |= ((uint32_t)(rand() & 0xFF)) << 16;
    val |= ((uint32_t)(rand() & 0xFF)) << 8;
    val |= ((uint32_t)(rand() & 0xFF));
    ip.value = val;
    return ip;
}