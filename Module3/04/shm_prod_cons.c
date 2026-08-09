#include "shm_prod_cons.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

static shm_status_t sem_lock(int semid) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) != 0) {
        return SHM_ERR_SEMOP;
    }
    return SHM_SUCCESS;
}

static shm_status_t sem_unlock(int semid) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) != 0) {
        return SHM_ERR_SEMOP;
    }
    return SHM_SUCCESS;
}

key_t get_shm_key(void) {
    key_t k = ftok(".", 'S');
    if (k == (key_t)-1) {
        k = 0x53484d34;
    }
    return k;
}

shm_status_t parse_args(int argc, char *argv[], config_t *config) {
    if (argc < 2 || argv == NULL || config == NULL) {
        return SHM_ERR_INVALID_ARGS;
    }

    config->ipc_key = get_shm_key();

    if (strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "--producer") == 0) {
        config->role = ROLE_PRODUCER;
        return SHM_SUCCESS;
    } else if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--consumer") == 0) {
        config->role = ROLE_CONSUMER;
        return SHM_SUCCESS;
    }

    return SHM_ERR_INVALID_ARGS;
}

shm_status_t run_producer(const config_t *config) {
    int shmid = shmget(config->ipc_key, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid < 0) {
        if (errno == EEXIST) {
            fprintf(stderr, "Producer error: shared memory already exists.\n");
            return SHM_ERR_SHMGET;
        }
        return SHM_ERR_SHMGET;
    }

    int semid = semget(config->ipc_key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid < 0) {
        shmctl(shmid, IPC_RMID, NULL);
        return SHM_ERR_SEMGET;
    }

    if (semctl(semid, 0, SETVAL, 1) < 0) {
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        return SHM_ERR_SEMOP;
    }

    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        return SHM_ERR_SHMAT;
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

        sem_lock(semid);

        if (hdr->free_offset + node_size > SHM_SIZE) {
            hdr->finished_producer = 1;
            sem_unlock(semid);
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

        sem_unlock(semid);

        usleep(15000);
    }

    while (1) {
        sem_lock(semid);
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
        sem_unlock(semid);

        if (all_processed) {
            break;
        }
        usleep(30000);
    }

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return SHM_SUCCESS;
}

shm_status_t run_consumer(const config_t *config) {
    int shmid = shmget(config->ipc_key, SHM_SIZE, 0666);
    if (shmid < 0) {
        fprintf(stderr, "Consumer error: shared memory not found.\n");
        return SHM_ERR_SHMGET;
    }

    int semid = semget(config->ipc_key, 1, 0666);
    if (semid < 0) {
        return SHM_ERR_SEMGET;
    }

    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        return SHM_ERR_SHMAT;
    }

    shm_header_t *hdr = (shm_header_t *)shm_ptr;
    pid_t cpid = getpid();

    while (1) {
        sem_lock(semid);

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

            sem_unlock(semid);
            usleep(20000);
        } else {
            int is_finished = hdr->finished_producer;
            sem_unlock(semid);

            if (is_finished) {
                break;
            }
            usleep(30000);
        }
    }

    shmdt(shm_ptr);
    return SHM_SUCCESS;
}