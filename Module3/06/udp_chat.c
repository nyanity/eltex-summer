#include "udp_chat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

static volatile sig_atomic_t g_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
}

static void send_broadcast(int sockfd, const struct sockaddr_in *bcast_addr, const char *msg) {
    sendto(sockfd, msg, strlen(msg), 0, (const struct sockaddr *)bcast_addr, sizeof(*bcast_addr));
}

static void *receiver_thread_func(void *arg) {
    udp_chat_config_t *config = (udp_chat_config_t *)arg;
    char buffer[MAX_MSG_LEN + 1];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    while (g_running) {
        ssize_t bytes = recvfrom(config->sockfd, buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&src_addr, &addr_len);
        if (bytes <= 0) {
            if (bytes < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
            break;
        }

        buffer[bytes] = '\0';

        pid_t msg_pid = 0;
        char nick[MAX_NICK_LEN] = {0};

        if (strncmp(buffer, "JOIN:", 5) == 0) {
            if (sscanf(buffer + 5, "%d:%31s", &msg_pid, nick) == 2) {
                if (msg_pid != config->pid) {
                    printf("\n[Chat] User '%s' (PID %d) joined the chat.\n> ", nick, msg_pid);
                    fflush(stdout);
                }
            }
        } else if (strncmp(buffer, "LEAVE:", 6) == 0) {
            if (sscanf(buffer + 6, "%d:%31s", &msg_pid, nick) == 2) {
                if (msg_pid != config->pid) {
                    printf("\n[Chat] User '%s' (PID %d) left the chat.\n> ", nick, msg_pid);
                    fflush(stdout);
                }
            }
        } else if (strncmp(buffer, "MSG:", 4) == 0) {
            char *ptr = buffer + 4;
            char *first_colon = strchr(ptr, ':');
            if (first_colon != NULL) {
                msg_pid = (pid_t)atoi(ptr);
                char *second_colon = strchr(first_colon + 1, ':');
                if (second_colon != NULL) {
                    size_t nlen = (size_t)(second_colon - (first_colon + 1));
                    if (nlen >= sizeof(nick)) nlen = sizeof(nick) - 1;
                    strncpy(nick, first_colon + 1, nlen);
                    nick[nlen] = '\0';

                    const char *payload = second_colon + 1;
                    if (msg_pid != config->pid) {
                        printf("\n[%s]: %s\n> ", nick, payload);
                        fflush(stdout);
                    }
                }
            }
        }
    }

    return NULL;
}

udp_chat_status_t parse_args(int argc, char *argv[], udp_chat_config_t *config) {
    if (config == NULL) {
        return UDP_CHAT_ERR_INVALID_ARGS;
    }

    config->pid = getpid();
    config->port = DEFAULT_PORT;
    config->sockfd = -1;

    if (argc >= 2 && argv[1] != NULL && strlen(argv[1]) > 0) {
        strncpy(config->nickname, argv[1], sizeof(config->nickname) - 1);
        config->nickname[sizeof(config->nickname) - 1] = '\0';
    } else {
        snprintf(config->nickname, sizeof(config->nickname), "User_%d", config->pid);
    }

    if (argc >= 3 && argv[2] != NULL) {
        char *endptr = NULL;
        long p = strtol(argv[2], &endptr, 10);
        if (*endptr != '\0' || p <= 0 || p > 65535) {
            return UDP_CHAT_ERR_INVALID_ARGS;
        }
        config->port = (uint16_t)p;
    }

    return UDP_CHAT_SUCCESS;
}

udp_chat_status_t run_udp_chat(udp_chat_config_t *config) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return UDP_CHAT_ERR_SOCKET;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(config->port);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        close(sockfd);
        return UDP_CHAT_ERR_BIND;
    }

    config->sockfd = sockfd;

    struct sockaddr_in bcast_addr;
    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family = AF_INET;
    bcast_addr.sin_port = htons(config->port);
    bcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receiver_thread_func, config) != 0) {
        close(sockfd);
        return UDP_CHAT_ERR_THREAD;
    }

    char packet[MAX_MSG_LEN];
    snprintf(packet, sizeof(packet), "JOIN:%d:%s", config->pid, config->nickname);
    send_broadcast(sockfd, &bcast_addr, packet);

    if (isatty(STDIN_FILENO)) {
        printf("Joined UDP broadcast chat as '%s' on port %u.\n", config->nickname, config->port);
        printf("Type messages and press Enter. Type /quit or Ctrl+C to exit.\n> ");
        fflush(stdout);

        char line[MAX_LINE_LEN];
        while (g_running && fgets(line, sizeof(line), stdin) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strlen(line) == 0) {
                printf("> ");
                fflush(stdout);
                continue;
            }

            if (strcmp(line, "/quit") == 0) {
                g_running = 0;
                break;
            }

            snprintf(packet, sizeof(packet), "MSG:%d:%s:%s", config->pid, config->nickname, line);
            send_broadcast(sockfd, &bcast_addr, packet);

            printf("> ");
            fflush(stdout);
        }
    } else {
        snprintf(packet, sizeof(packet), "MSG:%d:%s:Automated broadcast message", config->pid, config->nickname);
        send_broadcast(sockfd, &bcast_addr, packet);

        while (g_running) {
            usleep(50000);
        }
    }

    g_running = 0;
    snprintf(packet, sizeof(packet), "LEAVE:%d:%s", config->pid, config->nickname);
    send_broadcast(sockfd, &bcast_addr, packet);

    pthread_join(recv_thread, NULL);
    close(sockfd);

    return UDP_CHAT_SUCCESS;
}