#ifndef IPV4_SIM_H
#define IPV4_SIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IP_OK = 0,
    IP_ERR_INVALID_IP,
    IP_ERR_INVALID_MASK,
    IP_ERR_INVALID_COUNT
} ip_status_t;

typedef struct {
    uint32_t value;
} ipv4_addr_t;

ip_status_t parse_ip(const char *str, ipv4_addr_t *ip);
ip_status_t parse_mask(const char *str, ipv4_addr_t *mask);
int is_in_subnet(ipv4_addr_t ip, ipv4_addr_t gateway, ipv4_addr_t mask);
void ip_to_string(ipv4_addr_t ip, char *str);
ipv4_addr_t generate_random_ip(void);

#ifdef __cplusplus
}
#endif

#endif // IPV4_SIM_H