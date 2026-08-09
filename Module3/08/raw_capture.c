#include "raw_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

capture_status_t parse_args(int argc, char *argv[], capture_config_t *config) {
    if (argc < 3 || argv == NULL || config == NULL) {
        return CAPTURE_ERR_INVALID_ARGS;
    }

    if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--chat") == 0) {
        config->filter_type = FILTER_CHAT;
    } else if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--dns") == 0) {
        config->filter_type = FILTER_DNS;
    } else {
        return CAPTURE_ERR_INVALID_ARGS;
    }

    char *endptr = NULL;
    long p = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || p <= 0 || p > 65535) {
        return CAPTURE_ERR_INVALID_ARGS;
    }
    config->port = (uint16_t)p;

    config->max_packets = 10;
    if (argc >= 4) {
        long m = strtol(argv[3], &endptr, 10);
        if (*endptr == '\0' && m > 0) {
            config->max_packets = (int)m;
        }
    }

    return CAPTURE_SUCCESS;
}

capture_status_t run_capture(const capture_config_t *config) {
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        fprintf(stderr, "Socket creation failed (run as root/sudo): %s\n", strerror(errno));
        return CAPTURE_ERR_SOCKET;
    }

    char buffer[2048];
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("RAW capture started on port %u (max %d packets)...\n", config->port, config->max_packets);
    fflush(stdout);

    int captured = 0;
    while (captured < config->max_packets) {
        ssize_t bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (bytes < (ssize_t)(sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr))) {
            continue;
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;
        if (ntohs(eth->h_proto) != ETH_P_IP) {
            continue;
        }

        struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        if (ip->protocol != 17) {
            continue;
        }

        size_t ip_hdr_len = (size_t)(ip->ihl * 4);
        if (bytes < (ssize_t)(sizeof(struct ethhdr) + ip_hdr_len + sizeof(struct udphdr))) {
            continue;
        }

        struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_hdr_len);
        uint16_t src_port = ntohs(udp->source);
        uint16_t dst_port = ntohs(udp->dest);

        bool matches = false;
        const char *payload = buffer + sizeof(struct ethhdr) + ip_hdr_len + sizeof(struct udphdr);
        size_t payload_len = (size_t)(ntohs(udp->len) - sizeof(struct udphdr));

        if (config->filter_type == FILTER_CHAT) {
            if (src_port == config->port || dst_port == config->port) {
                if (payload_len >= 4 &&
                    (strncmp(payload, "JOIN:", 5) == 0 ||
                     strncmp(payload, "LEAVE:", 6) == 0 ||
                     strncmp(payload, "MSG:", 4) == 0)) {
                    matches = true;
                }
            }
        } else if (config->filter_type == FILTER_DNS) {
            if (src_port == config->port || dst_port == config->port) {
                matches = true;
            }
        }

        if (matches) {
            struct timespec curr_time;
            clock_gettime(CLOCK_MONOTONIC, &curr_time);
            double elapsed = (curr_time.tv_sec - start_time.tv_sec) +
                             (curr_time.tv_nsec - start_time.tv_nsec) / 1e9;

            char src_ip_str[INET_ADDRSTRLEN];
            char dst_ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ip->saddr), src_ip_str, sizeof(src_ip_str));
            inet_ntop(AF_INET, &(ip->daddr), dst_ip_str, sizeof(dst_ip_str));

            printf("[+%.4fs] ", elapsed);
            printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x | ",
                   eth->h_source[0], eth->h_source[1], eth->h_source[2],
                   eth->h_source[3], eth->h_source[4], eth->h_source[5],
                   eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
                   eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
            printf("IP: %s -> %s | ", src_ip_str, dst_ip_str);
            printf("PORT: %u -> %u | ", src_port, dst_port);

            printf("Payload (%zu bytes): ", payload_len);
            for (size_t i = 0; i < payload_len; ++i) {
                char c = payload[i];
                if (c >= 32 && c <= 126) {
                    putchar(c);
                } else {
                    printf("\\x%02x", (unsigned char)c);
                }
            }
            printf("\n");
            fflush(stdout);

            captured++;
        }
    }

    close(sockfd);
    return CAPTURE_SUCCESS;
}