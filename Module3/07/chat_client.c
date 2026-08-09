#include "chat_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
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

static void handle_file_send(int server_fd, const char *filepath) {
    int file_fd = open(filepath, O_RDONLY);
    if (file_fd < 0) {
        printf("Error opening file '%s'\n", filepath);
        return;
    }

    file_payload_t *payload = malloc(sizeof(file_payload_t));
    if (!payload) {
        close(file_fd);
        return;
    }

    memset(payload, 0, sizeof(*payload));
    const char *filename = strrchr(filepath, '/');
    if (filename == NULL) {
        filename = filepath;
    } else {
        filename++;
    }

    strncpy(payload->filename, filename, sizeof(payload->filename) - 1);

    ssize_t bytes = read(file_fd, payload->file_data, MAX_FILE_DATA);
    close(file_fd);

    if (bytes < 0) {
        printf("Error reading file content\n");
        free(payload);
        return;
    }

    payload->file_size = (uint32_t)bytes;

    msg_header_t hdr;
    hdr.type = MSG_FILE;
    hdr.length = sizeof(file_payload_t);

    if (send_all(server_fd, &hdr, sizeof(hdr)) == 0) {
        send_all(server_fd, payload, sizeof(file_payload_t));
        printf("File '%s' sent successfully!\n", filename);
    }

    free(payload);
}

int main(int argc, char *argv[]) {
    const char *ip = "127.0.0.1";
    int port = DEFAULT_PORT;
    char nickname[MAX_NICK_LEN] = "Anonymous";

    if (argc >= 2) {
        strncpy(nickname, argv[1], sizeof(nickname) - 1);
    }
    if (argc >= 3) {
        ip = argv[2];
    }
    if (argc >= 4) {
        port = atoi(argv[3]);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        printf("Connection failed.\n");
        return 1;
    }

    printf("Connected to server as '%s'. Type messages or '/file <filepath>' to share files.\n> ", nickname);
    fflush(stdout);

    fd_set read_fds;
    int monitor_stdin = 1;

    while (1) {
        FD_ZERO(&read_fds);
        if (monitor_stdin) {
            FD_SET(STDIN_FILENO, &read_fds);
        }
        FD_SET(server_fd, &read_fds);

        int max_fd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            msg_header_t hdr;
            if (recv_all(server_fd, &hdr, sizeof(hdr)) != 0) {
                printf("\nDisconnected from server.\n");
                break;
            }

            if (hdr.type == MSG_TEXT) {
                char *msg_text = malloc(hdr.length);
                if (msg_text) {
                    if (recv_all(server_fd, msg_text, hdr.length) == 0) {
                        printf("\n%s\n> ", msg_text);
                        fflush(stdout);
                    }
                    free(msg_text);
                }
            } else if (hdr.type == MSG_FILE) {
                file_payload_t *payload = malloc(sizeof(file_payload_t));
                if (payload) {
                    if (recv_all(server_fd, payload, sizeof(file_payload_t)) == 0) {
                        char out_name[512];
                        snprintf(out_name, sizeof(out_name), "%s.received", payload->filename);
                        int out_fd = open(out_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (out_fd >= 0) {
                            write(out_fd, payload->file_data, payload->file_size);
                            close(out_fd);
                            printf("\n[System] Received file '%s' and saved as '%s'\n> ", payload->filename, out_name);
                            fflush(stdout);
                        }
                    }
                    free(payload);
                }
            }
        }

        if (monitor_stdin && FD_ISSET(STDIN_FILENO, &read_fds)) {
            char line[512];
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

            if (strncmp(line, "/file ", 6) == 0) {
                handle_file_send(server_fd, line + 6);
            } else {
                char formatted_msg[1024];
                snprintf(formatted_msg, sizeof(formatted_msg), "[%s]: %s", nickname, line);

                msg_header_t hdr;
                hdr.type = MSG_TEXT;
                hdr.length = strlen(formatted_msg) + 1;

                send_all(server_fd, &hdr, sizeof(hdr));
                send_all(server_fd, formatted_msg, hdr.length);
            }
            printf("> ");
            fflush(stdout);
        }
    }

    close(server_fd);
    return 0;
}