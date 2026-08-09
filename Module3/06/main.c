#include "udp_chat.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    udp_chat_config_t config;
    udp_chat_status_t status = parse_args(argc, argv, &config);
    if (status != UDP_CHAT_SUCCESS) {
        fprintf(stderr, "Usage: %s [nickname] [port]\n", argv[0]);
        return 1;
    }

    status = run_udp_chat(&config);
    if (status != UDP_CHAT_SUCCESS) {
        fprintf(stderr, "Execution failed with status: %d\n", status);
        return 1;
    }

    return 0;
}