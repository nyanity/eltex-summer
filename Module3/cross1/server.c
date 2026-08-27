#include "raw_echo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_CLIENTS 128
static client_record_t clients[MAX_CLIENTS];
static int client_count = 0;
static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static uint32_t get_and_increment_client_count(uint32_t ip, uint16_t port, bool reset) {
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].ip == ip && clients[i].port == port) {
            if (reset) {
                clients[i].count = 0;
                return 0;
            }
            clients[i].count++;
            return clients[i].count;
        }
    }

    if (reset) {
        return 0;
    }

    if (client_count < MAX_CLIENTS) {
        clients[client_count].ip = ip;
        clients[client_count].port = port;
        clients[client_count].count = 1;
        client_count++;
        return 1;
    }

    return 1;
}

static int send_raw_udp(int sockfd, uint32_t src_port, uint32_t dst_ip, uint16_t dst_port, const char *payload, size_t payload_len) {
    char packet[2048];
    if (sizeof(struct udphdr) + payload_len > sizeof(packet)) {
        return -1;
    }

    struct udphdr *udp = (struct udphdr *)packet;
    udp->source = htons(src_port);
    udp->dest = htons(dst_port);
    udp->len = htons(sizeof(struct udphdr) + payload_len);
    udp->check = 0;

    memcpy(packet + sizeof(struct udphdr), payload, payload_len);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dst_port);
    dest_addr.sin_addr.s_addr = dst_ip;

    ssize_t sent = sendto(sockfd, packet, sizeof(struct udphdr) + payload_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    return (sent < 0) ? -1 : 0;
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    uint16_t server_port = DEFAULT_SERVER_PORT;
    if (argc >= 2) {
        server_port = (uint16_t)atoi(argv[1]);
    }

    int dummy_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (dummy_fd >= 0) {
        int opt = 1;
        setsockopt(dummy_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(dummy_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        struct sockaddr_in dummy_addr;
        memset(&dummy_addr, 0, sizeof(dummy_addr));
        dummy_addr.sin_family = AF_INET;
        dummy_addr.sin_port = htons(server_port);
        dummy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind(dummy_fd, (struct sockaddr *)&dummy_addr, sizeof(dummy_addr));
    }

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        fprintf(stderr, "Socket creation failed (run as root/sudo): %s\n", strerror(errno));
        if (dummy_fd >= 0) close(dummy_fd);
        return 1;
    }

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(server_port);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        close(sockfd);
        if (dummy_fd >= 0) close(dummy_fd);
        fprintf(stderr, "Bind failed: %s\n", strerror(errno));
        return 1;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
#if !defined(SO_RCVTIMEO) && defined(SO_RCVTIMEO_OLD)
    #define SO_RCVTIMEO SO_RCVTIMEO_OLD
#elif
    #define SO_RCVTIMEO 20
#endif
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("Raw Echo Server started on port %u...\n", server_port);
    fflush(stdout);

    char buffer[2048];
    while (g_running) {
        ssize_t bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            break;
        }

        if (bytes < (ssize_t)(sizeof(struct iphdr) + sizeof(struct udphdr))) {
            continue;
        }

        struct iphdr *ip = (struct iphdr *)buffer;
        size_t ip_hdr_len = ip->ihl * 4;
        if (bytes < (ssize_t)(ip_hdr_len + sizeof(struct udphdr))) {
            continue;
        }

        struct udphdr *udp = (struct udphdr *)(buffer + ip_hdr_len);
        uint16_t dst_port = ntohs(udp->dest);
        if (dst_port != server_port) {
            continue;
        }

        uint16_t src_port = ntohs(udp->source);
        uint32_t src_ip = ip->saddr;

        const char *payload = buffer + ip_hdr_len + sizeof(struct udphdr);
        size_t payload_len = ntohs(udp->len) - sizeof(struct udphdr);

        char msg[MAX_MSG_LEN];
        size_t m_len = (payload_len < sizeof(msg) - 1) ? payload_len : sizeof(msg) - 1;
        memcpy(msg, payload, m_len);
        msg[m_len] = '\0';

        if (strcmp(msg, CLOSE_CMD) == 0) {
            get_and_increment_client_count(src_ip, src_port, true);
            char src_ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &src_ip, src_ip_str, sizeof(src_ip_str));
            printf("Reset counters for client %s:%u\n", src_ip_str, src_port);
            fflush(stdout);
            continue;
        }

        uint32_t count = get_and_increment_client_count(src_ip, src_port, false);

        char response[MAX_MSG_LEN + 16];
        snprintf(response, sizeof(response), "%s %u", msg, count);

        send_raw_udp(sockfd, server_port, src_ip, src_port, response, strlen(response));
    }

    close(sockfd);
    if (dummy_fd >= 0) close(dummy_fd);
    printf("Server stopped.\n");
    return 0;
}