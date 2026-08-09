#ifndef POSIX_SHM_H
#define POSIX_SHM_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHM_SIZE 4096
#define SHM_NAME "/posix_shm_pcons"
#define SEM_NAME "/posix_sem_pcons"
#define ALIGN8(x) (((x) + (size_t)7) & ~((size_t)7))

typedef enum {
    POSIX_SHM_SUCCESS = 0,
    POSIX_SHM_ERR_INVALID_ARGS,
    POSIX_SHM_ERR_SHM_OPEN,
    POSIX_SHM_ERR_MMAP,
    POSIX_SHM_ERR_SEM_OPEN,
    POSIX_SHM_ERR_SEM_OP
} posix_shm_status_t;

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
} config_t;

posix_shm_status_t parse_args(int argc, char *argv[], config_t *config);
posix_shm_status_t run_producer(const config_t *config);
posix_shm_status_t run_consumer(const config_t *config);

#ifdef __cplusplus
}
#endif

#endif