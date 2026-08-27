#ifndef RAW_ECHO_H
#define RAW_ECHO_H

#include <stdint.h>
#include <sys/types.h>

#define DEFAULT_SERVER_PORT 9999
#define DEFAULT_CLIENT_PORT 8888
#define MAX_MSG_LEN 512
#define CLOSE_CMD "CLOSE_SESSION"

typedef struct {
    uint32_t ip;
    uint16_t port;
    uint32_t count;
} client_record_t;

#endif