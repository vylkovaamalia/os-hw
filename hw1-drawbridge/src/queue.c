#include "queue.h"
#include <stdlib.h>

bool queue_init(Queue *q, size_t capacity) {
    if (!q || capacity == 0)
        return false;

    q->data = malloc(capacity * sizeof(void *));
    if (!q->data)
        return false;

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    return true;
}

void queue_free(Queue *q) {
    if (!q) return;
    free(q->data);
    q->data = NULL;
    q->capacity = 0;
    q->head = 0;
    q->tail = 0;
    q->size = 0;
}

bool queue_push(Queue *q, void *item) {
    if (!q || queue_is_full(q))
        return false;

    q->data[q->tail] = item;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    return true;
}

void *queue_pop(Queue *q) {
    if (!q || queue_is_empty(q))
        return NULL;

    void *item = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return item;
}

void *queue_peek(const Queue *q) {
    if (!q || queue_is_empty(q))
        return NULL;
    return q->data[q->head];
}

bool queue_is_empty(const Queue *q) {
    return !q || q->size == 0;
}

bool queue_is_full(const Queue *q) {
    return q && q->size == q->capacity;
}

size_t queue_size(const Queue *q) {
    return q ? q->size : 0;
}
