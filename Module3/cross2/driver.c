#include "taxi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

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

void run_driver(int port) {
    pid_t pid = getpid();

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(client_fd);
        exit(1);
    }

    char init_msg[64];
    snprintf(init_msg, sizeof(init_msg), "INIT %d", pid);
    send_all(client_fd, init_msg, strlen(init_msg));

    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd < 0) {
        close(client_fd);
        exit(1);
    }

    bool is_busy = false;
    struct timespec task_start_time;
    int task_duration = 0;

    fd_set read_fds;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        FD_SET(timer_fd, &read_fds);

        int max_fd = (client_fd > timer_fd) ? client_fd : timer_fd;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (FD_ISSET(timer_fd, &read_fds)) {
            uint64_t expirations;
            if (read(timer_fd, &expirations, sizeof(expirations)) > 0) {
                is_busy = false;
                send_all(client_fd, "AVAILABLE", 9);
            }
        }

        if (FD_ISSET(client_fd, &read_fds)) {
            char cmd[BUFFER_SIZE];
            memset(cmd, 0, sizeof(cmd));
            ssize_t bytes = recv(client_fd, cmd, sizeof(cmd) - 1, 0);
            if (bytes <= 0) {
                break;
            }

            cmd[bytes] = '\0';

            if (strncmp(cmd, "TASK ", 5) == 0) {
                int timer_val = atoi(cmd + 5);
                if (is_busy) {
                    struct timespec curr_time;
                    clock_gettime(CLOCK_MONOTONIC, &curr_time);
                    double elapsed = (curr_time.tv_sec - task_start_time.tv_sec) +
                                     (curr_time.tv_nsec - task_start_time.tv_nsec) / 1e9;
                    int remaining = task_duration - (int)elapsed;
                    if (remaining < 0) remaining = 0;

                    char resp[64];
                    snprintf(resp, sizeof(resp), "Busy %d", remaining);
                    send_all(client_fd, resp, strlen(resp));
                } else {
                    is_busy = true;
                    task_duration = timer_val;
                    clock_gettime(CLOCK_MONOTONIC, &task_start_time);

                    struct itimerspec new_value;
                    new_value.it_value.tv_sec = timer_val;
                    new_value.it_value.tv_nsec = 0;
                    new_value.it_interval.tv_sec = 0;
                    new_value.it_interval.tv_nsec = 0;
                    timerfd_settime(timer_fd, 0, &new_value, NULL);

                    send_all(client_fd, "Available", 9);
                }
            } else if (strcmp(cmd, "STATUS") == 0) {
                if (is_busy) {
                    struct timespec curr_time;
                    clock_gettime(CLOCK_MONOTONIC, &curr_time);
                    double elapsed = (curr_time.tv_sec - task_start_time.tv_sec) +
                                     (curr_time.tv_nsec - task_start_time.tv_nsec) / 1e9;
                    int remaining = task_duration - (int)elapsed;
                    if (remaining < 0) remaining = 0;

                    char resp[64];
                    snprintf(resp, sizeof(resp), "Busy %d", remaining);
                    send_all(client_fd, resp, strlen(resp));
                } else {
                    send_all(client_fd, "Available", 9);
                }
            } else if (strcmp(cmd, "TERMINATE") == 0) {
                break;
            }
        }
    }

    close(timer_fd);
    close(client_fd);
}