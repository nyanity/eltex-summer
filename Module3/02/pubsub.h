#ifndef PUBSUB_H
#define PUBSUB_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MSG_SIZE 1024
#define MAX_TOPIC_LEN 64
#define MAX_PAYLOAD_LEN 256

typedef enum {
    ROLE_UNKNOWN = 0,
    ROLE_BROKER,
    ROLE_PUBLISHER,
    ROLE_SUBSCRIBER
} pubsub_role_t;

typedef enum {
    PUBSUB_SUCCESS = 0,
    PUBSUB_ERR_INVALID_ARGS,
    PUBSUB_ERR_QUEUE_EXISTS,
    PUBSUB_ERR_QUEUE_NOT_FOUND,
    PUBSUB_ERR_QUEUE_FAILED,
    PUBSUB_ERR_SEND_FAILED,
    PUBSUB_ERR_RECV_FAILED,
    PUBSUB_ERR_MEMORY
} pubsub_status_t;

typedef struct {
    long mtype;
    char mtext[MAX_MSG_SIZE];
} msg_buf_t;

typedef struct {
    pubsub_role_t role;
    char **topics;
    size_t topic_count;
    key_t msg_key;
} pubsub_config_t;

key_t get_default_key(void);
pubsub_status_t parse_args(int argc, char *argv[], pubsub_config_t *config);
pubsub_status_t run_broker(const pubsub_config_t *config);
pubsub_status_t run_publisher(const pubsub_config_t *config);
pubsub_status_t run_subscriber(const pubsub_config_t *config);

#ifdef __cplusplus
}
#endif

#endif