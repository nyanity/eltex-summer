#ifndef CHAT_PROTOCOL_H
#define CHAT_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_PORT 9999
#define MAX_NICK_LEN 32
#define MAX_FILE_DATA 32768

typedef enum {
    MSG_TEXT = 1,
    MSG_FILE = 2
} msg_type_t;

typedef struct {
    uint32_t type;
    uint32_t length;
} __attribute__((packed)) msg_header_t;

typedef struct {
    char filename[256];
    uint32_t file_size;
    char file_data[MAX_FILE_DATA];
} __attribute__((packed)) file_payload_t;

#ifdef __cplusplus
}
#endif

#endif