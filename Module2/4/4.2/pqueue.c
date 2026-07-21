#include "pqueue.h"
#include <stdlib.h>
#include <string.h>

pqueue_status_t pqueue_init(pqueue_t *q) {
    if (!q) return PQUEUE_ERR_INVALID_ARG;
    for (int i = 0; i < 256; i++) {
        q->buckets[i].head = NULL;
        q->buckets[i].tail = NULL;
    }
    q->size = 0;
    return PQUEUE_OK;
}

void pqueue_free(pqueue_t *q) {
    if (q) {
        for (int i = 0; i < 256; i++) {
            qnode_t *curr = q->buckets[i].head;
            while (curr) {
                qnode_t *next = curr->next;
                free(curr->text);
                free(curr);
                curr = next;
            }
            q->buckets[i].head = NULL;
            q->buckets[i].tail = NULL;
        }
        q->size = 0;
    }
}

pqueue_status_t pqueue_push(pqueue_t *q, const char *text, uint8_t priority) {
    if (!q || !text) return PQUEUE_ERR_INVALID_ARG;
    
    qnode_t *node = malloc(sizeof(qnode_t));
    if (!node) return PQUEUE_ERR_MEM;
    
    node->text = strdup(text);
    if (!node->text) {
        free(node);
        return PQUEUE_ERR_MEM;
    }
    node->priority = priority;
    node->next = NULL;

    fifo_queue_t *bucket = &q->buckets[priority];
    if (!bucket->head) {
        bucket->head = node;
        bucket->tail = node;
    } else {
        bucket->tail->next = node;
        bucket->tail = node;
    }
    q->size++;
    return PQUEUE_OK;
}

pqueue_status_t pqueue_pop_highest(pqueue_t *q, char **out_text, uint8_t *out_priority) {
    if (!q || !out_text) return PQUEUE_ERR_INVALID_ARG;
    
    for (int i = 255; i >= 0; i--) {
        fifo_queue_t *bucket = &q->buckets[i];
        if (bucket->head) {
            qnode_t *node = bucket->head;
            *out_text = node->text;
            if (out_priority) {
                *out_priority = node->priority;
            }
            bucket->head = node->next;
            if (!bucket->head) {
                bucket->tail = NULL;
            }
            free(node);
            q->size--;
            return PQUEUE_OK;
        }
    }
    return PQUEUE_ERR_EMPTY;
}

pqueue_status_t pqueue_pop_exact(pqueue_t *q, uint8_t priority, char **out_text) {
    if (!q || !out_text) return PQUEUE_ERR_INVALID_ARG;
    
    fifo_queue_t *bucket = &q->buckets[priority];
    if (!bucket->head) {
        return PQUEUE_ERR_EMPTY;
    }
    
    qnode_t *node = bucket->head;
    *out_text = node->text;
    bucket->head = node->next;
    if (!bucket->head) {
        bucket->tail = NULL;
    }
    free(node);
    q->size--;
    return PQUEUE_OK;
}

pqueue_status_t pqueue_pop_at_least(pqueue_t *q, uint8_t threshold, char **out_text, uint8_t *out_priority) {
    if (!q || !out_text) return PQUEUE_ERR_INVALID_ARG;
    
    for (int i = 255; i >= threshold; i--) {
        fifo_queue_t *bucket = &q->buckets[i];
        if (bucket->head) {
            qnode_t *node = bucket->head;
            *out_text = node->text;
            if (out_priority) {
                *out_priority = node->priority;
            }
            bucket->head = node->next;
            if (!bucket->head) {
                bucket->tail = NULL;
            }
            free(node);
            q->size--;
            return PQUEUE_OK;
        }
    }
    return PQUEUE_ERR_EMPTY;
}

size_t pqueue_get_size(const pqueue_t *q) {
    return q ? q->size : 0;
}