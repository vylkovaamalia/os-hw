#ifndef STREAM_H
#define STREAM_H

#include <stddef.h>
#include "shared.h"

int    shm_cars_push(SharedState *shm, int slot_idx, Priority prio);
int    shm_cars_pop (SharedState *shm, int *slot_idx);
size_t shm_cars_size(const SharedState *shm);
size_t shm_cars_size_priority(const SharedState *shm);

size_t      shm_ships_size          (const SharedState *shm, ShipDirection dir);
size_t      shm_ships_size_priority (const SharedState *shm, ShipDirection dir);
ShipDesc   *shm_ships_peek_priority (const SharedState *shm, ShipDirection dir);
ShipDesc   *shm_ships_peek_normal   (const SharedState *shm, ShipDirection dir);
ShipDesc   *shm_ships_pop_priority  (SharedState *shm, ShipDirection dir);
ShipDesc   *shm_ships_pop_normal    (SharedState *shm, ShipDirection dir);

#endif