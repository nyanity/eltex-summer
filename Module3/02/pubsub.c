#include "pubsub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <errno.h>

static volatile sig_atomic_t g_keep_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    g_keep_running = 0;
}

typedef struct {
    pid_t pid;
    char **topics;
    size_t topic_count;
} subscriber_entry_t;

typedef struct {
    subscriber_entry_t *entries;
    size_t count;
    size_t capacity;
} subscriber_list_t;

typedef struct {
    pid_t *pids;
    size_t count;
    size_t capacity;
} publisher_list_t;

key_t get_default_key(void) {
    key_t k = ftok(".", 'M');
    if (k == (key_t)-1) {
        k = 0x4d534751;
    }
    return k;
}

static void add_publisher(publisher_list_t *list, pid_t pid) {
    for (size_t i = 0; i < list->count; ++i) {
        if (list->pids[i] == pid) {
            return;
        }
    }
    if (list->count >= list->capacity) {
        size_t new_cap = (list->capacity == 0) ? 8 : list->capacity * 2;
        pid_t *temp = (pid_t *)realloc(list->pids, new_cap * sizeof(pid_t));
        if (!temp) {
            return;
        }
        list->pids = temp;
        list->capacity = new_cap;
    }
    list->pids[list->count++] = pid;
}

static void add_subscriber(subscriber_list_t *list, pid_t pid, const char *topic) {
    subscriber_entry_t *sub = NULL;
    for (size_t i = 0; i < list->count; ++i) {
        if (list->entries[i].pid == pid) {
            sub = &list->entries[i];
            break;
        }
    }

    if (sub == NULL) {
        if (list->count >= list->capacity) {
            size_t new_cap = (list->capacity == 0) ? 8 : list->capacity * 2;
            subscriber_entry_t *temp = (subscriber_entry_t *)realloc(list->entries, new_cap * sizeof(subscriber_entry_t));
            if (!temp) {
                return;
            }
            list->entries = temp;
            list->capacity = new_cap;
        }
        sub = &list->entries[list->count++];
        sub->pid = pid;
        sub->topics = NULL;
        sub->topic_count = 0;
    }

    for (size_t j = 0; j < sub->topic_count; ++j) {
        if (strcmp(sub->topics[j], topic) == 0) {
            return;
        }
    }

    char **new_topics = (char **)realloc(sub->topics, (sub->topic_count + 1) * sizeof(char *));
    if (!new_topics) {
        return;
    }
    sub->topics = new_topics;
    sub->topics[sub->topic_count] = strdup(topic);
    if (sub->topics[sub->topic_count]) {
        sub->topic_count++;
    }
}

static void remove_subscriber(subscriber_list_t *list, pid_t pid, const char *topic) {
    for (size_t i = 0; i < list->count; ++i) {
        if (list->entries[i].pid == pid) {
            subscriber_entry_t *sub = &list->entries[i];
            for (size_t j = 0; j < sub->topic_count; ++j) {
                if (strcmp(sub->topics[j], topic) == 0) {
                    free(sub->topics[j]);
                    for (size_t k = j; k < sub->topic_count - 1; ++k) {
                        sub->topics[k] = sub->topics[k + 1];
                    }
                    sub->topic_count--;
                    break;
                }
            }

            if (sub->topic_count == 0) {
                free(sub->topics);
                for (size_t k = i; k < list->count - 1; ++k) {
                    list->entries[k] = list->entries[k + 1];
                }
                list->count--;
            }
            break;
        }
    }
}

static void free_subscriber_list(subscriber_list_t *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        for (size_t j = 0; j < list->entries[i].topic_count; ++j) {
            free(list->entries[i].topics[j]);
        }
        free(list->entries[i].topics);
    }
    free(list->entries);
    list->entries = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void free_publisher_list(publisher_list_t *list) {
    if (!list) return;
    free(list->pids);
    list->pids = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void forward_message_to_subscribers(int msqid, const subscriber_list_t *subs, const char *topic, const char *text) {
    for (size_t i = 0; i < subs->count; ++i) {
        const subscriber_entry_t *sub = &subs->entries[i];
        int subscribed = 0;
        for (size_t j = 0; j < sub->topic_count; ++j) {
            if (strcmp(sub->topics[j], topic) == 0) {
                subscribed = 1;
                break;
            }
        }

        if (subscribed) {
            msg_buf_t out_msg;
            out_msg.mtype = (long)sub->pid;
            strncpy(out_msg.mtext, text, sizeof(out_msg.mtext) - 1);
            out_msg.mtext[sizeof(out_msg.mtext) - 1] = '\0';
            msgsnd(msqid, &out_msg, strlen(out_msg.mtext) + 1, IPC_NOWAIT);
        }
    }
}

static void process_broker_message(int msqid, const char *text, subscriber_list_t *subs, publisher_list_t *pubs) {
    char copy[MAX_MSG_SIZE];
    strncpy(copy, text, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    if (strncmp(copy, "subscribe,", 10) == 0) {
        pid_t pid = 0;
        char topic[MAX_TOPIC_LEN] = {0};
        if (sscanf(copy + 10, "%d,%63s", &pid, topic) == 2) {
            add_subscriber(subs, pid, topic);
        }
    } else if (strncmp(copy, "unsubscribe,", 12) == 0) {
        pid_t pid = 0;
        char topic[MAX_TOPIC_LEN] = {0};
        if (sscanf(copy + 12, "%d,%63s", &pid, topic) == 2) {
            remove_subscriber(subs, pid, topic);
        }
    } else if (strncmp(copy, "send,", 5) == 0) {
        pid_t pid = 0;
        char topic[MAX_TOPIC_LEN] = {0};
        char *ptr = copy + 5;
        char *first_comma = strchr(ptr, ',');
        if (first_comma != NULL) {
            pid = (pid_t)atoi(ptr);
            char *second_comma = strchr(first_comma + 1, ',');
            if (second_comma != NULL) {
                size_t topic_len = (size_t)(second_comma - (first_comma + 1));
                if (topic_len >= MAX_TOPIC_LEN) topic_len = MAX_TOPIC_LEN - 1;
                strncpy(topic, first_comma + 1, topic_len);
                topic[topic_len] = '\0';
            } else {
                strncpy(topic, first_comma + 1, sizeof(topic) - 1);
            }
            add_publisher(pubs, pid);
            forward_message_to_subscribers(msqid, subs, topic, text);
        }
    }
}

pubsub_status_t parse_args(int argc, char *argv[], pubsub_config_t *config) {
    if (argc < 2 || argv == NULL || config == NULL) {
        return PUBSUB_ERR_INVALID_ARGS;
    }

    config->role = ROLE_UNKNOWN;
    config->topics = NULL;
    config->topic_count = 0;
    config->msg_key = get_default_key();

    if (strcmp(argv[1], "-b") == 0) {
        config->role = ROLE_BROKER;
        return PUBSUB_SUCCESS;
    } else if (strcmp(argv[1], "-p") == 0) {
        if (argc < 3) {
            return PUBSUB_ERR_INVALID_ARGS;
        }
        config->role = ROLE_PUBLISHER;
        config->topics = &argv[2];
        config->topic_count = (size_t)(argc - 2);
        return PUBSUB_SUCCESS;
    } else if (strcmp(argv[1], "-s") == 0) {
        if (argc < 3) {
            return PUBSUB_ERR_INVALID_ARGS;
        }
        config->role = ROLE_SUBSCRIBER;
        config->topics = &argv[2];
        config->topic_count = (size_t)(argc - 2);
        return PUBSUB_SUCCESS;
    }

    return PUBSUB_ERR_INVALID_ARGS;
}

pubsub_status_t run_broker(const pubsub_config_t *config) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int msqid = msgget(config->msg_key, IPC_CREAT | IPC_EXCL | 0666);
    if (msqid < 0) {
        if (errno == EEXIST) {
            fprintf(stderr, "Broker is already running (queue exists).\n");
            return PUBSUB_ERR_QUEUE_EXISTS;
        }
        return PUBSUB_ERR_QUEUE_FAILED;
    }

    subscriber_list_t subs = {NULL, 0, 0};
    publisher_list_t pubs = {NULL, 0, 0};

    printf("Broker started (msqid=%d). Running...\n", msqid);
    fflush(stdout);

    while (g_keep_running) {
        msg_buf_t msg;
        ssize_t res = msgrcv(msqid, &msg, sizeof(msg.mtext), 1, IPC_NOWAIT);
        if (res < 0) {
            if (errno == ENOMSG) {
                usleep(10000);
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EIDRM || errno == EINVAL) {
                break;
            }
            break;
        }

        process_broker_message(msqid, msg.mtext, &subs, &pubs);
    }

    for (size_t i = 0; i < subs.count; ++i) {
        kill(subs.entries[i].pid, SIGINT);
    }
    for (size_t i = 0; i < pubs.count; ++i) {
        kill(pubs.pids[i], SIGINT);
    }

    free_subscriber_list(&subs);
    free_publisher_list(&pubs);

    msgctl(msqid, IPC_RMID, NULL);
    printf("Broker shutdown complete.\n");
    fflush(stdout);

    return PUBSUB_SUCCESS;
}

pubsub_status_t run_publisher(const pubsub_config_t *config) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int msqid = msgget(config->msg_key, 0666);
    if (msqid < 0) {
        fprintf(stderr, "Error: Message queue not available.\n");
        return PUBSUB_ERR_QUEUE_NOT_FOUND;
    }

    const char *topic = config->topics[0];
    pid_t pid = getpid();

    if (isatty(STDIN_FILENO)) {
        char line[MAX_PAYLOAD_LEN];
        printf("Publisher [%d] started for topic '%s'. Enter text:\n", pid, topic);
        fflush(stdout);
        while (g_keep_running && fgets(line, sizeof(line), stdin) != NULL) {
            line[strcspn(line, "\r\n")] = 0;
            if (strlen(line) == 0) continue;

            msg_buf_t msg;
            msg.mtype = 1;
            snprintf(msg.mtext, sizeof(msg.mtext), "send,%d,%s,%s", pid, topic, line);
            if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) < 0) {
                if (errno == EIDRM || errno == EINVAL) {
                    fprintf(stderr, "Queue unavailable. Publisher exiting.\n");
                    return PUBSUB_ERR_QUEUE_NOT_FOUND;
                }
            }
        }
    } else {
        msg_buf_t msg;
        msg.mtype = 1;
        snprintf(msg.mtext, sizeof(msg.mtext), "send,%d,%s,Automated payload", pid, topic);
        if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) < 0) {
            return PUBSUB_ERR_SEND_FAILED;
        }

        while (g_keep_running) {
            msgget(config->msg_key, 0666);
            usleep(50000);
        }
    }

    return PUBSUB_SUCCESS;
}

pubsub_status_t run_subscriber(const pubsub_config_t *config) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int msqid = msgget(config->msg_key, 0666);
    if (msqid < 0) {
        fprintf(stderr, "Error: Message queue not available.\n");
        return PUBSUB_ERR_QUEUE_NOT_FOUND;
    }

    pid_t pid = getpid();

    for (size_t i = 0; i < config->topic_count; ++i) {
        msg_buf_t msg;
        msg.mtype = 1;
        snprintf(msg.mtext, sizeof(msg.mtext), "subscribe,%d,%s", pid, config->topics[i]);
        if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) < 0) {
            return PUBSUB_ERR_SEND_FAILED;
        }
    }

    while (g_keep_running) {
        msg_buf_t msg;
        ssize_t res = msgrcv(msqid, &msg, sizeof(msg.mtext), (long)pid, IPC_NOWAIT);
        if (res < 0) {
            if (errno == ENOMSG) {
                usleep(20000);
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EIDRM || errno == EINVAL) {
                fprintf(stderr, "Queue unavailable. Subscriber exiting.\n");
                break;
            }
            break;
        }

        printf("[Subscriber %d] Received: %s\n", pid, msg.mtext);
        fflush(stdout);
    }

    int check_msqid = msgget(config->msg_key, 0666);
    if (check_msqid >= 0) {
        for (size_t i = 0; i < config->topic_count; ++i) {
            msg_buf_t msg;
            msg.mtype = 1;
            snprintf(msg.mtext, sizeof(msg.mtext), "unsubscribe,%d,%s", pid, config->topics[i]);
            msgsnd(check_msqid, &msg, strlen(msg.mtext) + 1, IPC_NOWAIT);
        }
    }

    return PUBSUB_SUCCESS;
}