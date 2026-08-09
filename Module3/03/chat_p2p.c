#include "chat_p2p.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

static volatile sig_atomic_t g_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
}

typedef struct {
    chat_session_t *session;
} thread_arg_t;

static void *receiver_thread_func(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    chat_session_t *session = targ->session;
    char buffer[MAX_MSG_SIZE + 1];

    while (g_running) {
        unsigned int prio = 0;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;

        ssize_t bytes = mq_timedreceive(session->mq_rx, buffer, MAX_MSG_SIZE, &prio, &ts);
        if (bytes < 0) {
            if (errno == ETIMEDOUT || errno == EINTR) {
                continue;
            }
            break;
        }

        buffer[bytes] = '\0';
        if (prio == PRIO_EXIT || strcmp(buffer, "EXIT") == 0) {
            printf("\nPeer has disconnected.\n");
            fflush(stdout);
            g_running = 0;
            break;
        }

        printf("\n[Peer]: %s\n> ", buffer);
        fflush(stdout);
    }

    return NULL;
}

chat_status_t parse_chat_args(int argc, char *argv[], char *base_name, size_t max_len) {
    if (argc < 2 || argv == NULL || base_name == NULL || max_len == 0) {
        return CHAT_ERR_INVALID_ARGS;
    }

    const char *input = argv[1];
    if (input[0] == '/') {
        snprintf(base_name, max_len, "%s", input);
    } else {
        snprintf(base_name, max_len, "/%s", input);
    }

    return CHAT_SUCCESS;
}

chat_status_t init_chat_session(const char *base_name, chat_session_t *session) {
    if (base_name == NULL || session == NULL) {
        return CHAT_ERR_INVALID_ARGS;
    }

    memset(session, 0, sizeof(*session));
    snprintf(session->base_name, sizeof(session->base_name), "%s", base_name);
    snprintf(session->qname1, sizeof(session->qname1), "%s_1", base_name);
    snprintf(session->qname2, sizeof(session->qname2), "%s_2", base_name);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mqd_t mq1 = mq_open(session->qname1, O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
    mqd_t mq2 = (mqd_t)-1;

    if (mq1 != (mqd_t)-1) {
        mq2 = mq_open(session->qname2, O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
        if (mq2 == (mqd_t)-1) {
            mq_close(mq1);
            mq_unlink(session->qname1);
            return CHAT_ERR_MQ_OPEN;
        }

        session->is_creator = 1;
        session->mq_rx = mq1;
        session->mq_tx = mq2;
    } else {
        if (errno == EEXIST) {
            mq1 = mq_open(session->qname1, O_WRONLY);
            mq2 = mq_open(session->qname2, O_RDONLY);
            if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1) {
                if (mq1 != (mqd_t)-1) mq_close(mq1);
                if (mq2 != (mqd_t)-1) mq_close(mq2);
                return CHAT_ERR_MQ_OPEN;
            }

            session->is_creator = 0;
            session->mq_tx = mq1;
            session->mq_rx = mq2;
        } else {
            return CHAT_ERR_MQ_OPEN;
        }
    }

    return CHAT_SUCCESS;
}

chat_status_t run_chat_loop(chat_session_t *session) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    g_running = 1;

    thread_arg_t targ = {session};
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receiver_thread_func, &targ) != 0) {
        return CHAT_ERR_THREAD;
    }

    if (isatty(STDIN_FILENO)) {
        printf("Chat started on queue '%s' (Creator: %s).\n", session->base_name, session->is_creator ? "Yes" : "No");
        printf("Type messages and press Enter. Type /quit or press Ctrl+C to exit.\n> ");
        fflush(stdout);

        char line[MAX_MSG_SIZE];
        while (g_running && fgets(line, sizeof(line), stdin) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strlen(line) == 0) {
                printf("> ");
                fflush(stdout);
                continue;
            }

            if (strcmp(line, "/quit") == 0) {
                g_running = 0;
                break;
            }

            if (mq_send(session->mq_tx, line, strlen(line) + 1, PRIO_NORMAL) != 0) {
                break;
            }

            printf("> ");
            fflush(stdout);
        }
    } else {
        const char *auto_msg = "Hello from peer!";
        mq_send(session->mq_tx, auto_msg, strlen(auto_msg) + 1, PRIO_NORMAL);
        while (g_running) {
            usleep(50000);
        }
    }

    g_running = 0;
    mq_send(session->mq_tx, "EXIT", 5, PRIO_EXIT);

    pthread_join(recv_thread, NULL);

    return CHAT_SUCCESS;
}

void cleanup_chat_session(chat_session_t *session) {
    if (session == NULL) return;

    if (session->mq_rx != 0 && session->mq_rx != (mqd_t)-1) {
        mq_close(session->mq_rx);
    }
    if (session->mq_tx != 0 && session->mq_tx != (mqd_t)-1) {
        mq_close(session->mq_tx);
    }

    if (session->is_creator) {
        mq_unlink(session->qname1);
        mq_unlink(session->qname2);
    }
}