#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    void   **data;
    size_t   capacity;
    size_t   head;
    size_t   tail;
    size_t   size;
} Queue;

bool   queue_init(Queue *q, size_t capacity);
void   queue_free(Queue *q);

bool   queue_push(Queue *q, void *item);
void  *queue_pop(Queue *q);
void  *queue_peek(const Queue *q);

bool   queue_is_empty(const Queue *q);
bool   queue_is_full(const Queue *q);
size_t queue_size(const Queue *q);

#endif /* QUEUE_H */
