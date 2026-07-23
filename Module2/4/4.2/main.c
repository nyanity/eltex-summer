#include "pqueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void safe_get_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        } else {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
    }
}

static int get_int_option(const char *prompt) {
    char buffer[32];
    safe_get_line(prompt, buffer, sizeof(buffer));
    char *endptr;
    long val = strtol(buffer, &endptr, 10);
    if (endptr == buffer || *endptr != '\0') {
        return -1;
    }
    return (int)val;
}

int main(void) {
    pqueue_t q;
    if (pqueue_init(&q) != PQUEUE_OK) {
        fprintf(stderr, "Fatal error: failed to initialize priority queue.\n");
        return 1;
    }

    while (1) {
        printf("\n========================================\n");
        printf("             PRIORITY QUEUE             \n");
        printf("========================================\n");
        printf("1. Push Message (Add to Queue)\n");
        printf("2. Pop Highest Priority Message\n");
        printf("3. Pop Exact Priority Message\n");
        printf("4. Pop Message with At Least Priority...\n");
        printf("5. Print Queue Size\n");
        printf("6. Print Queue\n");
        printf("7. Exit\n");
        printf("========================================\n");

        int choice = get_int_option("Choose an option (1-7): ");
        if (choice == 7) {
            printf("\nExiting.\n");
            break;
        }

        char buffer[128];
        char *text = NULL;
        uint8_t priority = 0;
        pqueue_status_t status;

        switch (choice) {
            case 1:
                safe_get_line("Enter message text: ", buffer, sizeof(buffer));
                if (strlen(buffer) == 0) {
                    printf("Error: Message text cannot be empty.\n");
                    break;
                }
                
                int prio = get_int_option("Enter priority (0-255): ");
                if (prio < 0 || prio > 255) {
                    printf("Error: Priority must be in the range [0, 255].\n");
                    break;
                }

                status = pqueue_push(&q, buffer, (uint8_t)prio);
                if (status == PQUEUE_OK) {
                    printf("Message successfully added to queue.\n");
                } else {
                    printf("Error: Out of memory.\n");
                }
                break;

            case 2:
                status = pqueue_pop_highest(&q, &text, &priority);
                if (status == PQUEUE_OK) {
                    printf("Popped: \"%s\" (Priority: %d)\n", text, priority);
                    free(text);
                } else {
                    printf("Queue is empty.\n");
                }
                break;

            case 3:
                prio = get_int_option("Enter exact priority to pop (0-255): ");
                if (prio < 0 || prio > 255) {
                    printf("Error: Priority must be in the range [0, 255].\n");
                    break;
                }

                status = pqueue_pop_exact(&q, (uint8_t)prio, &text);
                if (status == PQUEUE_OK) {
                    printf("Popped (Exact %d): \"%s\"\n", prio, text);
                    free(text);
                } else {
                    printf("No message found with exact priority %d.\n", prio);
                }
                break;

            case 4:
                prio = get_int_option("Enter minimum priority threshold (0-255): ");
                if (prio < 0 || prio > 255) {
                    printf("Error: Priority must be in the range [0, 255].\n");
                    break;
                }

                status = pqueue_pop_at_least(&q, (uint8_t)prio, &text, &priority);
                if (status == PQUEUE_OK) {
                    printf("Popped: \"%s\" (Priority: %d, matched >= %d)\n", text, priority, prio);
                    free(text);
                } else {
                    printf("No message found with priority >= %d.\n", prio);
                }
                break;

            case 5:
                printf("Current Queue Size: %zu\n", pqueue_get_size(&q));
                break;

            case 6:
                prio = get_int_option("Enter order(1: Lowest to highest, 2: Highest to lowest): ");
                switch(prio) {
                    case 1: pqueue_print(&q, LOW_TO_HIGH); break;
                    case 2: pqueue_print(&q, HIGH_TO_LOW); break;
                    default: printf("There is no that order scheme.\n"); break;
                }
                break;

            default:
                printf("Invalid selection. Please try again.\n");
                break;
        }
    }

    pqueue_free(&q);
    return 0;
}