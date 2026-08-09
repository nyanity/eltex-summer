#include "chat_p2p.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    char base_name[MAX_NAME_LEN];
    chat_status_t status = parse_chat_args(argc, argv, base_name, sizeof(base_name));
    if (status != CHAT_SUCCESS) {
        fprintf(stderr, "Usage: %s <queue_name>\n", argv[0]);
        return 1;
    }

    chat_session_t session;
    status = init_chat_session(base_name, &session);
    if (status != CHAT_SUCCESS) {
        fprintf(stderr, "Failed to initialize chat session: %d\n", status);
        return 1;
    }

    run_chat_loop(&session);
    cleanup_chat_session(&session);

    return 0;
}