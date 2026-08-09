#ifndef UDP_CHAT_H
#define UDP_CHAT_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_PORT 8888
#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 32
#define MAX_LINE_LEN 256

typedef enum {
    UDP_CHAT_SUCCESS = 0,
    UDP_CHAT_ERR_INVALID_ARGS,
    UDP_CHAT_ERR_SOCKET,
    UDP_CHAT_ERR_BIND,
    UDP_CHAT_ERR_SEND,
    UDP_CHAT_ERR_THREAD
} udp_chat_status_t;

typedef struct {
    char nickname[MAX_NICK_LEN];
    uint16_t port;
    int sockfd;
    pid_t pid;
} udp_chat_config_t;

udp_chat_status_t parse_args(int argc, char *argv[], udp_chat_config_t *config);
udp_chat_status_t run_udp_chat(udp_chat_config_t *config);

#ifdef __cplusplus
}
#endif

#endif