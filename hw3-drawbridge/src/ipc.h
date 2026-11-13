#ifndef IPC_H
#define IPC_H

#include <semaphore.h>
#include "shared.h"

SharedState *ipc_setup(void);
SharedState *ipc_attach(void);

extern sem_t *g_mutex;
extern sem_t *g_log_mutex;
extern sem_t *g_event;
extern sem_t *g_mech;

void mutex_lock(void);
void mutex_unlock(void);

void ipc_cleanup(void);

static inline void ring_init(RingHeader *r, size_t capacity) {
    r->head = r->tail = r->size = 0;
    r->capacity = capacity;
}

static inline int ring_push(RingHeader *r, int *idx, int value) {
    if (r->size >= r->capacity) return -1;
    idx[r->tail] = value;
    r->tail = (r->tail + 1) % r->capacity;
    r->size++;
    return 0;
}

static inline int ring_pop(RingHeader *r, int *idx, int *out) {
    if (r->size == 0) return -1;
    *out = idx[r->head];
    r->head = (r->head + 1) % r->capacity;
    r->size--;
    return 0;
}

static inline int ring_peek(const RingHeader *r, const int *idx, int *out) {
    if (r->size == 0) return -1;
    *out = idx[r->head];
    return 0;
}

#endif