#ifndef RAW_CAPTURE_H
#define RAW_CAPTURE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAPTURE_SUCCESS = 0,
    CAPTURE_ERR_INVALID_ARGS,
    CAPTURE_ERR_SOCKET,
    CAPTURE_ERR_BIND,
    CAPTURE_ERR_RECV
} capture_status_t;

typedef enum {
    FILTER_CHAT = 1,
    FILTER_DNS = 2
} filter_type_t;

typedef struct {
    filter_type_t filter_type;
    uint16_t port;
    int max_packets;
} capture_config_t;

capture_status_t parse_args(int argc, char *argv[], capture_config_t *config);
capture_status_t run_capture(const capture_config_t *config);

#ifdef __cplusplus
}
#endif

#endif