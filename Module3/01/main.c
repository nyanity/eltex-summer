#include "copy_app.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    app_config_t config;
    copy_status_t status = parse_args(argc, argv, &config);
    if (status != COPY_SUCCESS) {
        fprintf(stderr, "Usage: %s [-p fifo_name] <file1> [file2 ...]\n", argv[0]);
        return 1;
    }

    status = run_copy_process(&config);
    if (status != COPY_SUCCESS) {
        fprintf(stderr, "File copying failed with status code: %d\n", status);
        return 1;
    }

    return 0;
}