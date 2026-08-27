#ifndef TAXI_H
#define TAXI_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define SERVER_PORT 9090
#define MAX_DRIVERS 64
#define BUFFER_SIZE 256

typedef struct {
    pid_t pid;
    int fd;
} driver_record_t;

#endif