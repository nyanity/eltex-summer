#include "ipv4_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <gateway_ip> <subnet_mask> <packet_count>\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.1 255.255.255.0 1000\n", argv[0]);
        return 1;
    }

    ipv4_addr_t gateway;
    if (parse_ip(argv[1], &gateway) != IP_OK) {
        fprintf(stderr, "Error: Invalid gateway IP address '%s'\n", argv[1]);
        return 1;
    }

    ipv4_addr_t mask;
    if (parse_mask(argv[2], &mask) != IP_OK) {
        fprintf(stderr, "Error: Invalid subnet mask '%s'\n", argv[2]);
        return 1;
    }

    char *endptr;
    long n_packets = strtol(argv[3], &endptr, 10);
    if (endptr == argv[3] || *endptr != '\0' || n_packets <= 0) {
        fprintf(stderr, "Error: Invalid packet count '%s'. Must be a positive integer.\n", argv[3]);
        return 1;
    }

    srand((unsigned int)time(NULL));

    long local_count = 0;
    long remote_count = 0;

    printf("Starting simulation of %ld packets...\n", n_packets);
    printf("Subnet: Gateway=%s, Mask=%s\n\n", argv[1], argv[2]);

    for (long i = 0; i < n_packets; i++) {
        ipv4_addr_t dest_ip;
    
        if (rand() % 100 < 30) {
            ipv4_addr_t rand_host = generate_random_ip();
            dest_ip.value = (gateway.value & mask.value) | (rand_host.value & ~mask.value);
        } else {
            dest_ip = generate_random_ip();
        }

        char dest_ip_str[16];
        ip_to_string(dest_ip, dest_ip_str);

        if (is_in_subnet(dest_ip, gateway, mask)) {
            local_count++;
            if (n_packets <= 20) {
                printf("Packet [%ld]: Dest=%s -> Local subnet (Forward directly)\n", i + 1, dest_ip_str);
            }
        } else {
            remote_count++;
            if (n_packets <= 20) {
                printf("Packet [%ld]: Dest=%s -> External network (Forward to Gateway)\n", i + 1, dest_ip_str);
            }
        }
    }

    if (n_packets > 20) {
        printf("...[Statistics truncated for brevity]...\n\n");
    }

    double local_percent = ((double)local_count / n_packets) * 100.0;
    double remote_percent = ((double)remote_count / n_packets) * 100.0;

    printf("=== SIMULATION STATISTICS ===\n");
    printf("Total Packets Processed:  %ld\n", n_packets);
    printf("Local Subnet Packets:     %ld (%.2f%%)\n", local_count, local_percent);
    printf("External Network Packets:  %ld (%.2f%%)\n", remote_count, remote_percent);
    printf("=============================\n");

    return 0;
}