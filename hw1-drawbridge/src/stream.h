#ifndef STREAM_H
#define STREAM_H

#include <stdbool.h>
#include <stddef.h>
#include "queue.h"

typedef struct {
   Queue  normal;
   Queue  priority;
   size_t capacity;
} Stream;

bool   stream_init(Stream *s, size_t capacity);
void   stream_free(Stream *s);

bool   stream_push(Stream *s, void *item, bool is_priority);
void  *stream_pop(Stream *s);
const void  *stream_peek(const Stream *s);

bool   stream_is_empty(const Stream *s);
bool   stream_is_full(const Stream *s);
size_t stream_size(const Stream *s);

size_t stream_size_normal(const Stream *s);
size_t stream_size_priority(const Stream *s);

void *stream_pop_priority(Stream *s);
void *stream_pop_normal(Stream *s);

const void *stream_peek_priority(const Stream *s);
const void *stream_peek_normal(const Stream *s);

#endif /* STREAM_H */
