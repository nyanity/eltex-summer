#include "raw_capture.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    capture_config_t config;
    capture_status_t status = parse_args(argc, argv, &config);
    if (status != CAPTURE_SUCCESS) {
        fprintf(stderr, "Usage: %s <filter_type> <port> [max_packets]\n", argv[0]);
        fprintf(stderr, "Filter types:\n");
        fprintf(stderr, "  -c, --chat   Task 6 Chat Messages\n");
        fprintf(stderr, "  -d, --dns    General UDP/DNS filter\n");
        return 1;
    }

    status = run_capture(&config);
    if (status != CAPTURE_SUCCESS) {
        fprintf(stderr, "Capture failed with status: %d\n", status);
        return 1;
    }

    return 0;
}