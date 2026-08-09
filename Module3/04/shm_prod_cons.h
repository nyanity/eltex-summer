#ifndef SHM_PROD_CONS_H
#define SHM_PROD_CONS_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHM_SIZE 4096
#define ALIGN8(x) (((x) + (size_t)7) & ~((size_t)7))

typedef enum {
    SHM_SUCCESS = 0,
    SHM_ERR_INVALID_ARGS,
    SHM_ERR_SHMGET,
    SHM_ERR_SHMAT,
    SHM_ERR_SEMGET,
    SHM_ERR_SEMOP,
    SHM_ERR_EXHAUSTED
} shm_status_t;

typedef enum {
    ROLE_PRODUCER = 1,
    ROLE_CONSUMER = 2
} app_role_t;

typedef struct {
    int finished_producer;
    size_t head_offset;
    size_t tail_offset;
    size_t free_offset;
} shm_header_t;

typedef struct {
    size_t count;
    size_t next_offset;
} node_header_t;

typedef struct {
    app_role_t role;
    key_t ipc_key;
} config_t;

key_t get_shm_key(void);
shm_status_t parse_args(int argc, char *argv[], config_t *config);
shm_status_t run_producer(const config_t *config);
shm_status_t run_consumer(const config_t *config);

#ifdef __cplusplus
}
#endif

#endif