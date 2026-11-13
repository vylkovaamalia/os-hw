#include "stream.h"
#include "ipc.h"

int shm_cars_push(SharedState *shm, int slot_idx, Priority prio) {
    if (prio == EMERGENCY) {
        return ring_push(&shm->cars_priority,
                         shm->cars_priority_idx, slot_idx);
    }
    return ring_push(&shm->cars_normal,
                     shm->cars_normal_idx, slot_idx);
}

int shm_cars_pop(SharedState *shm, int *slot_idx) {
    if (ring_pop(&shm->cars_priority,
                 shm->cars_priority_idx, slot_idx) == 0) {
        return 0;
    }
    return ring_pop(&shm->cars_normal,
                    shm->cars_normal_idx, slot_idx);
}

size_t shm_cars_size(const SharedState *shm) {
    return shm->cars_priority.size + shm->cars_normal.size;
}

size_t shm_cars_size_priority(const SharedState *shm) {
    return shm->cars_priority.size;
}

static RingHeader *ships_ring(SharedState *shm, ShipDirection dir,
                              int priority, int **idx_out)
{
    if (dir == SHIP_UP) {
        if (priority) { *idx_out = shm->ships_up_priority_idx;
                        return &shm->ships_up_priority; }
        *idx_out = shm->ships_up_normal_idx;
        return &shm->ships_up_normal;
    }
    if (priority) { *idx_out = shm->ships_down_priority_idx;
                    return &shm->ships_down_priority; }
    *idx_out = shm->ships_down_normal_idx;
    return &shm->ships_down_normal;
}

static ShipDesc *ship_slot(SharedState *shm, int idx) {
    if (idx < 0 || idx >= MAX_SHIPS) return NULL;
    return &shm->ship_slots[idx];
}

/* ищем в ring первый живой слот, мёртвые (used=0) пропускаем */
static ShipDesc *pop_live(SharedState *shm, ShipDirection dir, int priority) {
    int *idx;
    RingHeader *r = ships_ring(shm, dir, priority, &idx);
    int out;
    while (ring_pop(r, idx, &out) == 0) {
        ShipDesc *s = ship_slot(shm, out);
        if (s && s->used) return s;
    }
    return NULL;
}

static ShipDesc *peek_live(SharedState *shm, ShipDirection dir, int priority) {
    int *idx;
    RingHeader *r = ships_ring(shm, dir, priority, &idx);
    int out;
    if (ring_peek(r, idx, &out) < 0) return NULL;
    ShipDesc *s = ship_slot(shm, out);
    if (s && !s->used) {
        /* мёртвая голова, снимаем и пробуем снова */
        ring_pop(r, idx, &out);
        return peek_live(shm, dir, priority);
    }
    return s;
}

size_t shm_ships_size(const SharedState *shm, ShipDirection dir) {
    if (dir == SHIP_UP)
        return shm->ships_up_priority.size + shm->ships_up_normal.size;
    return     shm->ships_down_priority.size + shm->ships_down_normal.size;
}

size_t shm_ships_size_priority(const SharedState *shm, ShipDirection dir) {
    return (dir == SHIP_UP) ? shm->ships_up_priority.size
                            : shm->ships_down_priority.size;
}

ShipDesc *shm_ships_peek_priority(const SharedState *shm, ShipDirection dir) {
    return peek_live((SharedState *)shm, dir, 1);
}

ShipDesc *shm_ships_peek_normal(const SharedState *shm, ShipDirection dir) {
    return peek_live((SharedState *)shm, dir, 0);
}

ShipDesc *shm_ships_pop_priority(SharedState *shm, ShipDirection dir) {
    return pop_live(shm, dir, 1);
}

ShipDesc *shm_ships_pop_normal(SharedState *shm, ShipDirection dir) {
    return pop_live(shm, dir, 0);
}