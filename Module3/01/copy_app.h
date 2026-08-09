#ifndef COPY_APP_H
#define COPY_APP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COPY_SUCCESS = 0,
    COPY_ERR_INVALID_ARGS,
    COPY_ERR_FORK_FAILED,
    COPY_ERR_PIPE_FAILED,
    COPY_ERR_FILE_NOT_FOUND,
    COPY_ERR_FILE_READ,
    COPY_ERR_FILE_WRITE,
    COPY_ERR_IPC,
    COPY_ERR_CHILD_FAILED
} copy_status_t;

typedef enum {
    MSG_TYPE_HEADER = 1,
    MSG_TYPE_SKIP = 2,
    MSG_TYPE_DONE = 3
} msg_type_t;

#define MAX_PATH_LEN 256
#define BUFFER_SIZE 4096

typedef struct {
    int32_t msg_type;
    char filename[MAX_PATH_LEN];
    int64_t filesize;
} file_header_t;

typedef struct {
    const char *fifo_path;
    char **filenames;
    size_t file_count;
} app_config_t;

copy_status_t parse_args(int argc, char *argv[], app_config_t *config);
copy_status_t run_copy_process(const app_config_t *config);

#ifdef __cplusplus
}
#endif

#endif