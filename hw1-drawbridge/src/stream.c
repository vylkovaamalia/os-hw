#include "stream.h"

bool stream_init(Stream *s, size_t capacity) {
    if (!s || capacity == 0)
        return false;

    if (!queue_init(&s->normal,   capacity))
        return false;
    if (!queue_init(&s->priority, capacity)) {
        queue_free(&s->normal);
        return false;
    }

    s->capacity = capacity;
    return true;
}

void stream_free(Stream *s) {
    if (!s) return;
    queue_free(&s->normal);
    queue_free(&s->priority);
    s->capacity = 0;
}

bool stream_push(Stream *s, void *item, bool is_priority) {
    if (!s || stream_is_full(s))
        return false;

    Queue *target = is_priority ? &s->priority : &s->normal;
    return queue_push(target, item);
}

void *stream_pop(Stream *s) {
    if (!s) return NULL;

    if (!queue_is_empty(&s->priority))
        return queue_pop(&s->priority);
    if (!queue_is_empty(&s->normal))
        return queue_pop(&s->normal);
    return NULL;
}

const void *stream_peek(const Stream *s) {
    if (!s) return NULL;

    if (!queue_is_empty(&s->priority))
        return queue_peek(&s->priority);
    if (!queue_is_empty(&s->normal))
        return queue_peek(&s->normal);
    return NULL;
}

bool stream_is_empty(const Stream *s) {
    return !s || (queue_is_empty(&s->priority) && queue_is_empty(&s->normal));
}

bool stream_is_full(const Stream *s) {
    return s && stream_size(s) >= s->capacity;
}

size_t stream_size(const Stream *s) {
    if (!s) return 0;
    return queue_size(&s->normal) + queue_size(&s->priority);
}

size_t stream_size_normal(const Stream *s) {
    return s ? queue_size(&s->normal) : 0;
}

size_t stream_size_priority(const Stream *s) {
    return s ? queue_size(&s->priority) : 0;
}

void *stream_pop_priority(Stream *s) {
    return s ? queue_pop(&s->priority) : NULL;
}

void *stream_pop_normal(Stream *s) {
    return s ? queue_pop(&s->normal) : NULL;
}

const void *stream_peek_priority(const Stream *s) {
    return s ? queue_peek(&s->priority) : NULL;
}

const void *stream_peek_normal(const Stream *s) {
    return s ? queue_peek(&s->normal) : NULL;
}
