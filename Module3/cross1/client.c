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
#include <sys/select.h>
#include <errno.h>

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
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

    const char *server_ip_str = "127.0.0.1";
    uint16_t server_port = DEFAULT_SERVER_PORT;
    uint16_t client_port = DEFAULT_CLIENT_PORT;

    if (argc >= 2) {
        client_port = (uint16_t)atoi(argv[1]);
    }
    if (argc >= 3) {
        server_ip_str = argv[2];
    }
    if (argc >= 4) {
        server_port = (uint16_t)atoi(argv[3]);
    }

    uint32_t server_ip = inet_addr(server_ip_str);

    int dummy_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (dummy_fd >= 0) {
        int opt = 1;
        setsockopt(dummy_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
        setsockopt(dummy_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
        struct sockaddr_in dummy_addr;
        memset(&dummy_addr, 0, sizeof(dummy_addr));
        dummy_addr.sin_family = AF_INET;
        dummy_addr.sin_port = htons(client_port);
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
    bind_addr.sin_port = htons(client_port);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        close(sockfd);
        if (dummy_fd >= 0) close(dummy_fd);
        fprintf(stderr, "Bind failed: %s\n", strerror(errno));
        return 1;
    }

    printf("Raw Echo Client started on port %u. Connected to %s:%u\n> ", client_port, server_ip_str, server_port);
    fflush(stdout);

    fd_set read_fds;
    int monitor_stdin = 1;

    char buffer[2048];

    while (g_running) {
        FD_ZERO(&read_fds);
        if (monitor_stdin) {
            FD_SET(STDIN_FILENO, &read_fds);
        }
        FD_SET(sockfd, &read_fds);

        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            ssize_t bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            if (bytes >= (ssize_t)(sizeof(struct iphdr) + sizeof(struct udphdr))) {
                struct iphdr *ip = (struct iphdr *)buffer;
                size_t ip_hdr_len = ip->ihl * 4;
                struct udphdr *udp = (struct udphdr *)(buffer + ip_hdr_len);
                uint16_t dst_port = ntohs(udp->dest);

                if (dst_port == client_port) {
                    const char *payload = buffer + ip_hdr_len + sizeof(struct udphdr);
                    size_t payload_len = ntohs(udp->len) - sizeof(struct udphdr);

                    char msg[MAX_MSG_LEN];
                    size_t m_len = (payload_len < sizeof(msg) - 1) ? payload_len : sizeof(msg) - 1;
                    memcpy(msg, payload, m_len);
                    msg[m_len] = '\0';

                    printf("\n[Server]: %s\n> ", msg);
                    fflush(stdout);
                }
            }
        }

        if (monitor_stdin && FD_ISSET(STDIN_FILENO, &read_fds)) {
            char line[256];
            if (fgets(line, sizeof(line), stdin) == NULL) {
                monitor_stdin = 0;
                continue;
            }

            line[strcspn(line, "\r\n")] = '\0';
            if (strlen(line) == 0) {
                printf("> ");
                fflush(stdout);
                continue;
            }

            send_raw_udp(sockfd, client_port, server_ip, server_port, line, strlen(line));
            printf("> ");
            fflush(stdout);
        }
    }

    send_raw_udp(sockfd, client_port, server_ip, server_port, CLOSE_CMD, strlen(CLOSE_CMD));
    close(sockfd);
    if (dummy_fd >= 0) close(dummy_fd);
    printf("\nClient stopped cleanly.\n");
    return 0;
}