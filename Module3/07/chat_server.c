#include "chat_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_CLIENTS 32

typedef struct {
    int fd;
    char nickname[MAX_NICK_LEN];
} client_t;

static client_t clients[MAX_CLIENTS];
static struct pollfd fds[MAX_CLIENTS + 1];
static int client_count = 0;
static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static int send_all(int fd, const void *buf, size_t len) {
    const char *ptr = (const char *)buf;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t sent = send(fd, ptr, remaining, 0);
        if (sent <= 0) {
            if (sent < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
            return -1;
        }
        ptr += sent;
        remaining -= sent;
    }
    return 0;
}

static int recv_all(int fd, void *buf, size_t len) {
    char *ptr = (char *)buf;
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t recved = recv(fd, ptr, remaining, 0);
        if (recved <= 0) {
            if (recved < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
            return -1;
        }
        ptr += recved;
        remaining -= recved;
    }
    return 0;
}

static void broadcast(const msg_header_t *hdr, const void *payload, int sender_fd) {
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].fd != sender_fd) {
            if (send_all(clients[i].fd, hdr, sizeof(*hdr)) == 0) {
                send_all(clients[i].fd, payload, hdr->length);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int port = DEFAULT_PORT;
    if (argc >= 2) {
        port = atoi(argv[1]);
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 10) < 0) {
        close(listen_fd);
        return 1;
    }

    printf("Server started on port %d...\n", port);

    memset(clients, 0, sizeof(clients));
    memset(fds, 0, sizeof(fds));

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;

    while (g_running) {
        for (int i = 0; i < client_count; ++i) {
            fds[i + 1].fd = clients[i].fd;
            fds[i + 1].events = POLLIN;
        }

        int poll_count = poll(fds, client_count + 1, -1);
        if (poll_count < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd >= 0) {
                if (client_count < MAX_CLIENTS) {
                    clients[client_count].fd = client_fd;
                    snprintf(clients[client_count].nickname, MAX_NICK_LEN, "User_%d", client_fd);
                    client_count++;
                    printf("New client connected: fd=%d\n", client_fd);
                } else {
                    close(client_fd);
                }
            }
        }

        for (int i = 0; i < client_count; ++i) {
            if (fds[i + 1].revents & POLLIN) {
                msg_header_t hdr;
                if (recv_all(clients[i].fd, &hdr, sizeof(hdr)) != 0) {
                    printf("Client disconnected: fd=%d\n", clients[i].fd);
                    close(clients[i].fd);
                    for (int j = i; j < client_count - 1; ++j) {
                        clients[j] = clients[j + 1];
                    }
                    client_count--;
                    i--;
                    continue;
                }

                char *payload = malloc(hdr.length);
                if (payload) {
                    if (recv_all(clients[i].fd, payload, hdr.length) == 0) {
                        broadcast(&hdr, payload, clients[i].fd);
                    }
                    free(payload);
                }
            }
        }
    }

    for (int i = 0; i < client_count; ++i) {
        close(clients[i].fd);
    }
    close(listen_fd);
    printf("Server stopped cleanly.\n");
    return 0;
}