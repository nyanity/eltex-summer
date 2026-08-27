#include "taxi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <errno.h>

static driver_record_t drivers[MAX_DRIVERS];
static int driver_count = 0;

void run_driver(int port);

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

static void handle_new_connection(int listen_fd, int epoll_fd) {
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        return;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    if (recv(client_fd, buffer, sizeof(buffer) - 1, 0) <= 0) {
        close(client_fd);
        return;
    }

    pid_t driver_pid = 0;
    if (sscanf(buffer, "INIT %d", &driver_pid) == 1) {
        if (driver_count < MAX_DRIVERS) {
            drivers[driver_count].pid = driver_pid;
            drivers[driver_count].fd = client_fd;
            driver_count++;

            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = client_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

            printf("\n[System] Driver %d connected and registered.\n> ", driver_pid);
            fflush(stdout);
        } else {
            send_all(client_fd, "TERMINATE", 9);
            close(client_fd);
        }
    } else {
        close(client_fd);
    }
}

static void handle_driver_message(int fd, int epoll_fd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        for (int i = 0; i < driver_count; ++i) {
            if (drivers[i].fd == fd) {
                printf("\n[System] Driver %d disconnected.\n> ", drivers[i].pid);
                fflush(stdout);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                for (int j = i; j < driver_count - 1; ++j) {
                    drivers[j] = drivers[j + 1];
                }
                driver_count--;
                break;
            }
        }
        return;
    }

    buffer[bytes] = '\0';
    if (strcmp(buffer, "AVAILABLE") == 0) {
        for (int i = 0; i < driver_count; ++i) {
            if (drivers[i].fd == fd) {
                printf("\n[Notification] Driver %d completed task and is now Available.\n> ", drivers[i].pid);
                fflush(stdout);
                break;
            }
        }
    }
}

static void handle_cli_input(int listen_fd) {
    char line[256];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return;
    }
    line[strcspn(line, "\r\n")] = '\0';
    if (strlen(line) == 0) {
        printf("> ");
        fflush(stdout);
        return;
    }

    if (strcmp(line, "create_driver") == 0) {
        pid_t pid = fork();
        if (pid < 0) {
            printf("Error: fork failed.\n");
        } else if (pid == 0) {
            close(listen_fd);
            run_driver(SERVER_PORT);
            exit(0);
        } else {
            printf("Spawning driver process %d...\n", pid);
        }
    } else if (strncmp(line, "send_task ", 10) == 0) {
        pid_t target_pid = 0;
        int timer_val = 0;
        if (sscanf(line + 10, "%d %d", &target_pid, &timer_val) == 2) {
            int found = 0;
            for (int i = 0; i < driver_count; ++i) {
                if (drivers[i].pid == target_pid) {
                    found = 1;
                    char cmd[64];
                    snprintf(cmd, sizeof(cmd), "TASK %d", timer_val);
                    send_all(drivers[i].fd, cmd, strlen(cmd));

                    char resp[BUFFER_SIZE];
                    memset(resp, 0, sizeof(resp));
                    ssize_t r = recv(drivers[i].fd, resp, sizeof(resp) - 1, 0);
                    if (r > 0) {
                        resp[r] = '\0';
                        printf("%s\n", resp);
                    }
                    break;
                }
            }
            if (!found) {
                printf("Error: Driver %d not found.\n", target_pid);
            }
        } else {
            printf("Usage: send_task <pid> <task_timer>\n");
        }
    } else if (strncmp(line, "get_status ", 11) == 0) {
        pid_t target_pid = 0;
        if (sscanf(line + 11, "%d", &target_pid) == 1) {
            int found = 0;
            for (int i = 0; i < driver_count; ++i) {
                if (drivers[i].pid == target_pid) {
                    found = 1;
                    send_all(drivers[i].fd, "STATUS", 6);

                    char resp[BUFFER_SIZE];
                    memset(resp, 0, sizeof(resp));
                    ssize_t r = recv(drivers[i].fd, resp, sizeof(resp) - 1, 0);
                    if (r > 0) {
                        resp[r] = '\0';
                        printf("%s\n", resp);
                    }
                    break;
                }
            }
            if (!found) {
                printf("Error: Driver %d not found.\n", target_pid);
            }
        } else {
            printf("Usage: get_status <pid>\n");
        }
    } else if (strcmp(line, "get_drivers") == 0) {
        if (driver_count == 0) {
            printf("No drivers registered.\n");
        } else {
            for (int i = 0; i < driver_count; ++i) {
                send_all(drivers[i].fd, "STATUS", 6);
                char resp[BUFFER_SIZE];
                memset(resp, 0, sizeof(resp));
                ssize_t r = recv(drivers[i].fd, resp, sizeof(resp) - 1, 0);
                if (r > 0) {
                    resp[r] = '\0';
                    printf("PID: %d | Status: %s\n", drivers[i].pid, resp);
                }
            }
        }
    } else {
        printf("Unknown command. Available commands:\n");
        printf("  create_driver\n");
        printf("  send_task <pid> <task_timer>\n");
        printf("  get_status <pid>\n");
        printf("  get_drivers\n");
    }

    printf("> ");
    fflush(stdout);
}

int main(void) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 10) < 0) {
        close(listen_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        close(listen_fd);
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    printf("Taxi Control Center started. Enter commands:\n> ");
    fflush(stdout);

    struct epoll_event events[MAX_DRIVERS + 2];

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_DRIVERS + 2, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                handle_new_connection(listen_fd, epoll_fd);
            } else if (fd == STDIN_FILENO) {
                handle_cli_input(listen_fd);
            } else {
                handle_driver_message(fd, epoll_fd);
            }
        }
    }

    for (int i = 0; i < driver_count; ++i) {
        send_all(drivers[i].fd, "TERMINATE", 9);
        close(drivers[i].fd);
        kill(drivers[i].pid, SIGTERM);
    }
    close(listen_fd);
    close(epoll_fd);
    return 0;
}