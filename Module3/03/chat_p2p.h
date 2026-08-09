#ifndef CHAT_P2P_H
#define CHAT_P2P_H

#include <stddef.h>
#include <mqueue.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MSG_SIZE 256
#define MAX_NAME_LEN 128
#define PRIO_NORMAL 1
#define PRIO_EXIT 10

typedef enum {
    CHAT_SUCCESS = 0,
    CHAT_ERR_INVALID_ARGS,
    CHAT_ERR_MQ_OPEN,
    CHAT_ERR_MQ_SEND,
    CHAT_ERR_MQ_RECV,
    CHAT_ERR_THREAD
} chat_status_t;

typedef struct {
    char base_name[MAX_NAME_LEN];
    char qname1[MAX_NAME_LEN];
    char qname2[MAX_NAME_LEN];
    mqd_t mq_rx;
    mqd_t mq_tx;
    int is_creator;
} chat_session_t;

chat_status_t parse_chat_args(int argc, char *argv[], char *base_name, size_t max_len);
chat_status_t init_chat_session(const char *base_name, chat_session_t *session);
chat_status_t run_chat_loop(chat_session_t *session);
void cleanup_chat_session(chat_session_t *session);

#ifdef __cplusplus
}
#endif

#endif