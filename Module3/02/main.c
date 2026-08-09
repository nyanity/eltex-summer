#include "pubsub.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    pubsub_config_t config;
    pubsub_status_t status = parse_args(argc, argv, &config);
    if (status != PUBSUB_SUCCESS) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  Broker:     %s -b\n", argv[0]);
        fprintf(stderr, "  Publisher:  %s -p <topic>\n", argv[0]);
        fprintf(stderr, "  Subscriber: %s -s <topic1> [topic2 ...]\n", argv[0]);
        return 1;
    }

    switch (config.role) {
        case ROLE_BROKER:
            status = run_broker(&config);
            break;
        case ROLE_PUBLISHER:
            status = run_publisher(&config);
            break;
        case ROLE_SUBSCRIBER:
            status = run_subscriber(&config);
            break;
        default:
            status = PUBSUB_ERR_INVALID_ARGS;
            break;
    }

    if (status != PUBSUB_SUCCESS) {
        fprintf(stderr, "Execution failed with status code: %d\n", status);
        return 1;
    }

    return 0;
}