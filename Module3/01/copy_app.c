#include "copy_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

static copy_status_t write_all(int fd, const void *buf, size_t count) {
    const char *ptr = (const char *)buf;
    size_t remaining = count;
    while (remaining > 0) {
        ssize_t written = write(fd, ptr, remaining);
        if (written <= 0) {
            if (written < 0 && errno == EINTR) {
                continue;
            }
            return COPY_ERR_IPC;
        }
        ptr += written;
        remaining -= (size_t)written;
    }
    return COPY_SUCCESS;
}

static copy_status_t read_all(int fd, void *buf, size_t count) {
    char *ptr = (char *)buf;
    size_t remaining = count;
    while (remaining > 0) {
        ssize_t bytes_read = read(fd, ptr, remaining);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return COPY_ERR_IPC;
        }
        if (bytes_read == 0) {
            return COPY_ERR_IPC;
        }
        ptr += bytes_read;
        remaining -= (size_t)bytes_read;
    }
    return COPY_SUCCESS;
}

static copy_status_t parent_process(int read_fd, int write_fd, const app_config_t *config) {
    char buffer[BUFFER_SIZE];
    for (size_t i = 0; i < config->file_count; ++i) {
        uint8_t ready = 0;
        if (read_all(read_fd, &ready, 1) != COPY_SUCCESS) {
            return COPY_ERR_IPC;
        }

        const char *filename = config->filenames[i];
        int src_fd = open(filename, O_RDONLY);
        file_header_t header;
        memset(&header, 0, sizeof(header));
        strncpy(header.filename, filename, sizeof(header.filename) - 1);

        if (src_fd < 0) {
            fprintf(stderr, "Error opening source file '%s': %s\n", filename, strerror(errno));
            header.msg_type = MSG_TYPE_SKIP;
            header.filesize = 0;
            if (write_all(write_fd, &header, sizeof(header)) != COPY_SUCCESS) {
                return COPY_ERR_IPC;
            }
            continue;
        }

        struct stat st;
        if (fstat(src_fd, &st) != 0) {
            fprintf(stderr, "Error stating source file '%s': %s\n", filename, strerror(errno));
            close(src_fd);
            header.msg_type = MSG_TYPE_SKIP;
            header.filesize = 0;
            if (write_all(write_fd, &header, sizeof(header)) != COPY_SUCCESS) {
                return COPY_ERR_IPC;
            }
            continue;
        }

        header.msg_type = MSG_TYPE_HEADER;
        header.filesize = (int64_t)st.st_size;

        if (write_all(write_fd, &header, sizeof(header)) != COPY_SUCCESS) {
            close(src_fd);
            return COPY_ERR_IPC;
        }

        int64_t bytes_left = header.filesize;
        while (bytes_left > 0) {
            size_t chunk = (bytes_left > BUFFER_SIZE) ? BUFFER_SIZE : (size_t)bytes_left;
            ssize_t read_bytes = read(src_fd, buffer, chunk);
            if (read_bytes <= 0) {
                if (read_bytes < 0 && errno == EINTR) {
                    continue;
                }
                close(src_fd);
                return COPY_ERR_FILE_READ;
            }
            if (write_all(write_fd, buffer, (size_t)read_bytes) != COPY_SUCCESS) {
                close(src_fd);
                return COPY_ERR_IPC;
            }
            bytes_left -= read_bytes;
        }
        close(src_fd);
    }

    uint8_t ready = 0;
    if (read_all(read_fd, &ready, 1) != COPY_SUCCESS) {
        return COPY_ERR_IPC;
    }

    file_header_t done_header;
    memset(&done_header, 0, sizeof(done_header));
    done_header.msg_type = MSG_TYPE_DONE;
    if (write_all(write_fd, &done_header, sizeof(done_header)) != COPY_SUCCESS) {
        return COPY_ERR_IPC;
    }

    return COPY_SUCCESS;
}

static copy_status_t child_process(int read_fd, int write_fd) {
    char buffer[BUFFER_SIZE];
    while (1) {
        uint8_t ready = 1;
        if (write_all(write_fd, &ready, 1) != COPY_SUCCESS) {
            return COPY_ERR_IPC;
        }

        file_header_t header;
        if (read_all(read_fd, &header, sizeof(header)) != COPY_SUCCESS) {
            return COPY_ERR_IPC;
        }

        if (header.msg_type == MSG_TYPE_DONE) {
            break;
        }

        if (header.msg_type == MSG_TYPE_SKIP) {
            continue;
        }

        if (header.msg_type == MSG_TYPE_HEADER) {
            char out_path[MAX_PATH_LEN + 10];
            snprintf(out_path, sizeof(out_path), "%s.copy", header.filename);

            int dst_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (dst_fd < 0) {
                return COPY_ERR_FILE_WRITE;
            }

            int64_t bytes_left = header.filesize;
            copy_status_t status = COPY_SUCCESS;

            while (bytes_left > 0) {
                size_t chunk = (bytes_left > BUFFER_SIZE) ? BUFFER_SIZE : (size_t)bytes_left;
                if (read_all(read_fd, buffer, chunk) != COPY_SUCCESS) {
                    status = COPY_ERR_IPC;
                    break;
                }

                if (write_all(dst_fd, buffer, chunk) != COPY_SUCCESS) {
                    status = COPY_ERR_FILE_WRITE;
                    break;
                }

                bytes_left -= (int64_t)chunk;
            }

            close(dst_fd);

            if (status != COPY_SUCCESS) {
                return status;
            }
        }
    }
    return COPY_SUCCESS;
}

copy_status_t parse_args(int argc, char *argv[], app_config_t *config) {
    if (argc < 2 || argv == NULL || config == NULL) {
        return COPY_ERR_INVALID_ARGS;
    }

    config->fifo_path = NULL;
    config->filenames = NULL;
    config->file_count = 0;

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                return COPY_ERR_INVALID_ARGS;
            }
            config->fifo_path = argv[i + 1];
            i += 2;
        } else {
            break;
        }
    }

    if (i >= argc) {
        return COPY_ERR_INVALID_ARGS;
    }

    config->filenames = &argv[i];
    config->file_count = (size_t)(argc - i);

    return COPY_SUCCESS;
}

copy_status_t run_copy_process(const app_config_t *config) {
    if (config == NULL || config->filenames == NULL || config->file_count == 0) {
        return COPY_ERR_INVALID_ARGS;
    }

    if (config->fifo_path != NULL) {
        char fifo_p2c[MAX_PATH_LEN];
        char fifo_c2p[MAX_PATH_LEN];

        snprintf(fifo_p2c, sizeof(fifo_p2c), "%s_p2c", config->fifo_path);
        snprintf(fifo_c2p, sizeof(fifo_c2p), "%s_c2p", config->fifo_path);

        unlink(fifo_p2c);
        unlink(fifo_c2p);

        if (mkfifo(fifo_p2c, 0666) != 0 || mkfifo(fifo_c2p, 0666) != 0) {
            unlink(fifo_p2c);
            unlink(fifo_c2p);
            return COPY_ERR_PIPE_FAILED;
        }

        pid_t pid = fork();
        if (pid < 0) {
            unlink(fifo_p2c);
            unlink(fifo_c2p);
            return COPY_ERR_FORK_FAILED;
        }

        if (pid == 0) {
            int rfd = open(fifo_p2c, O_RDONLY);
            int wfd = open(fifo_c2p, O_WRONLY);
            if (rfd < 0 || wfd < 0) {
                if (rfd >= 0) close(rfd);
                if (wfd >= 0) close(wfd);
                _exit(EXIT_FAILURE);
            }

            copy_status_t status = child_process(rfd, wfd);
            close(rfd);
            close(wfd);
            _exit(status == COPY_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
        } else {
            int wfd = open(fifo_p2c, O_WRONLY);
            int rfd = open(fifo_c2p, O_RDONLY);
            if (wfd < 0 || rfd < 0) {
                if (wfd >= 0) close(wfd);
                if (rfd >= 0) close(rfd);
                unlink(fifo_p2c);
                unlink(fifo_c2p);
                return COPY_ERR_PIPE_FAILED;
            }

            copy_status_t status = parent_process(rfd, wfd, config);
            close(rfd);
            close(wfd);
            unlink(fifo_p2c);
            unlink(fifo_c2p);

            int wstatus = 0;
            waitpid(pid, &wstatus, 0);
            if (status == COPY_SUCCESS && (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0)) {
                return COPY_ERR_CHILD_FAILED;
            }
            return status;
        }
    } else {
        int p2c[2];
        int c2p[2];

        if (pipe(p2c) != 0 || pipe(c2p) != 0) {
            return COPY_ERR_PIPE_FAILED;
        }

        pid_t pid = fork();
        if (pid < 0) {
            close(p2c[0]); close(p2c[1]);
            close(c2p[0]); close(c2p[1]);
            return COPY_ERR_FORK_FAILED;
        }

        if (pid == 0) {
            close(p2c[1]);
            close(c2p[0]);
            copy_status_t status = child_process(p2c[0], c2p[1]);
            close(p2c[0]);
            close(c2p[1]);
            _exit(status == COPY_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
        } else {
            close(p2c[0]);
            close(c2p[1]);
            copy_status_t status = parent_process(c2p[0], p2c[1], config);
            close(p2c[1]);
            close(c2p[0]);

            int wstatus = 0;
            waitpid(pid, &wstatus, 0);
            if (status == COPY_SUCCESS && (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0)) {
                return COPY_ERR_CHILD_FAILED;
            }
            return status;
        }
    }
}