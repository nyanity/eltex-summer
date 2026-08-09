#include "posix_shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

posix_shm_status_t parse_args(int argc, char *argv[], config_t *config) {
    if (argc < 2 || argv == NULL || config == NULL) {
        return POSIX_SHM_ERR_INVALID_ARGS;
    }

    if (strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "--producer") == 0) {
        config->role = ROLE_PRODUCER;
        return POSIX_SHM_SUCCESS;
    } else if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--consumer") == 0) {
        config->role = ROLE_CONSUMER;
        return POSIX_SHM_SUCCESS;
    }

    return POSIX_SHM_ERR_INVALID_ARGS;
}

posix_shm_status_t run_producer(const config_t *config) {
    (void)config;

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd < 0) {
        fprintf(stderr, "Producer error: shm_open failed: %s\n", strerror(errno));
        return POSIX_SHM_ERR_SHM_OPEN;
    }

    if (ftruncate(fd, SHM_SIZE) != 0) {
        close(fd);
        shm_unlink(SHM_NAME);
        return POSIX_SHM_ERR_SHM_OPEN;
    }

    char *shm_ptr = (char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (shm_ptr == MAP_FAILED) {
        shm_unlink(SHM_NAME);
        return POSIX_SHM_ERR_MMAP;
    }

    sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        return POSIX_SHM_ERR_SEM_OPEN;
    }

    shm_header_t *hdr = (shm_header_t *)shm_ptr;
    hdr->finished_producer = 0;
    hdr->head_offset = 0;
    hdr->tail_offset = 0;
    hdr->free_offset = ALIGN8(sizeof(shm_header_t));

    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

    while (1) {
        size_t count = (size_t)(rand() % 10 + 1);
        size_t node_size = ALIGN8(sizeof(node_header_t) + count * sizeof(int));

        if (sem_wait(sem) != 0) {
            break;
        }

        if (hdr->free_offset + node_size > SHM_SIZE) {
            hdr->finished_producer = 1;
            sem_post(sem);
            break;
        }

        size_t new_offset = hdr->free_offset;
        node_header_t *node = (node_header_t *)(shm_ptr + new_offset);
        node->count = count;
        node->next_offset = 0;

        int *data = (int *)(shm_ptr + new_offset + sizeof(node_header_t));
        for (size_t i = 0; i < count; ++i) {
            data[i] = rand() % 2000 - 1000;
        }

        if (hdr->head_offset == 0) {
            hdr->head_offset = new_offset;
        } else {
            node_header_t *tail = (node_header_t *)(shm_ptr + hdr->tail_offset);
            tail->next_offset = new_offset;
        }
        hdr->tail_offset = new_offset;
        hdr->free_offset += node_size;

        sem_post(sem);
        usleep(15000);
    }

    while (1) {
        if (sem_wait(sem) == 0) {
            int all_processed = 1;
            size_t curr_off = hdr->head_offset;
            while (curr_off != 0) {
                node_header_t *node = (node_header_t *)(shm_ptr + curr_off);
                if (node->count > 0) {
                    all_processed = 0;
                    break;
                }
                curr_off = node->next_offset;
            }
            sem_post(sem);

            if (all_processed) {
                break;
            }
        }
        usleep(30000);
    }

    munmap(shm_ptr, SHM_SIZE);
    sem_close(sem);
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    return POSIX_SHM_SUCCESS;
}

posix_shm_status_t run_consumer(const config_t *config) {
    (void)config;

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd < 0) {
        fprintf(stderr, "Consumer error: shm_open failed: %s\n", strerror(errno));
        return POSIX_SHM_ERR_SHM_OPEN;
    }

    char *shm_ptr = (char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (shm_ptr == MAP_FAILED) {
        return POSIX_SHM_ERR_MMAP;
    }

    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        munmap(shm_ptr, SHM_SIZE);
        return POSIX_SHM_ERR_SEM_OPEN;
    }

    shm_header_t *hdr = (shm_header_t *)shm_ptr;
    pid_t cpid = getpid();

    while (1) {
        if (sem_wait(sem) != 0) {
            break;
        }

        size_t curr_off = hdr->head_offset;
        node_header_t *target_node = NULL;
        size_t target_off = 0;

        while (curr_off != 0) {
            node_header_t *node = (node_header_t *)(shm_ptr + curr_off);
            if (node->count > 0) {
                target_node = node;
                target_off = curr_off;
                break;
            }
            curr_off = node->next_offset;
        }

        if (target_node != NULL) {
            size_t count = target_node->count;
            int *data = (int *)(shm_ptr + target_off + sizeof(node_header_t));
            int min_val = data[0];
            int max_val = data[0];
            for (size_t i = 1; i < count; ++i) {
                if (data[i] < min_val) min_val = data[i];
                if (data[i] > max_val) max_val = data[i];
            }

            printf("[Consumer %d] Processed node at offset %zu (%zu items): Min = %d, Max = %d\n",
                   cpid, target_off, count, min_val, max_val);
            fflush(stdout);

            target_node->count = 0;

            sem_post(sem);
            usleep(20000);
        } else {
            int is_finished = hdr->finished_producer;
            sem_post(sem);

            if (is_finished) {
                break;
            }
            usleep(30000);
        }
    }

    munmap(shm_ptr, SHM_SIZE);
    sem_close(sem);
    return POSIX_SHM_SUCCESS;
}