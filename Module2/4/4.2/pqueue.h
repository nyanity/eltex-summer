#ifndef PQUEUE_H
#define PQUEUE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PQUEUE_OK = 0,
    PQUEUE_ERR_MEM,
    PQUEUE_ERR_EMPTY,
    PQUEUE_ERR_INVALID_ARG
} pqueue_status_t;

typedef enum {
    HIGH_TO_LOW = 0,
    LOW_TO_HIGH
} pqueue_print_order_t;

typedef struct qnode {
    char *text;
    uint8_t priority;
    struct qnode *next;
} qnode_t;

typedef struct {
    qnode_t *head;
    qnode_t *tail;
} fifo_queue_t;

typedef struct {
    fifo_queue_t buckets[256];
    size_t size;
} pqueue_t;

pqueue_status_t pqueue_init(pqueue_t *q);
void pqueue_free(pqueue_t *q);

pqueue_status_t pqueue_push(pqueue_t *q, const char *text, uint8_t priority);

pqueue_status_t pqueue_pop_highest(pqueue_t *q, char **out_text, uint8_t *out_priority);

pqueue_status_t pqueue_pop_exact(pqueue_t *q, uint8_t priority, char **out_text);

pqueue_status_t pqueue_pop_at_least(pqueue_t *q, uint8_t threshold, char **out_text, uint8_t *out_priority);

void pqueue_print(pqueue_t *q, pqueue_print_order_t order);

size_t pqueue_get_size(const pqueue_t *q);

#ifdef __cplusplus
}
#endif

#endif