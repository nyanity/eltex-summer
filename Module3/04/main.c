#include "shm_prod_cons.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    config_t config;
    shm_status_t status = parse_args(argc, argv, &config);
    if (status != SHM_SUCCESS) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  Producer: %s -p\n", argv[0]);
        fprintf(stderr, "  Consumer: %s -c\n", argv[0]);
        return 1;
    }

    if (config.role == ROLE_PRODUCER) {
        status = run_producer(&config);
    } else {
        status = run_consumer(&config);
    }

    if (status != SHM_SUCCESS) {
        fprintf(stderr, "Execution failed with status: %d\n", status);
        return 1;
    }

    return 0;
}