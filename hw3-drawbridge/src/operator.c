#include "operator.h"
#include "bridge.h"
#include "stream.h"

static int ship_slot_index(SharedState *shm, ShipDesc *s) {
    return (int)(s - shm->ship_slots);
}

void operator_form_ship_group(SharedState *shm) {
    size_t max_size = shm->cfg.max_group_size;
    if (max_size > MAX_GROUP) max_size = MAX_GROUP;

    size_t n = 0;

    if (shm->cfg.group_strategy == GROUP_STRATEGY_PRIORITY) {
        /* сначала приоритетные обоих направлений */
        while (n < max_size) {
            ShipDesc *pu = shm_ships_peek_priority(shm, SHIP_UP);
            ShipDesc *pd = shm_ships_peek_priority(shm, SHIP_DOWN);
            if (!pu && !pd) break;
            ShipDirection dir;
            if (pu && (!pd || pu->arrival_time <= pd->arrival_time))
                dir = SHIP_UP;
            else
                dir = SHIP_DOWN;
            ShipDesc *s = shm_ships_pop_priority(shm, dir);
            if (!s) break;
            s->group_id = (int)n;
            s->state    = SHIP_IN_GROUP;
            shm->ship_group[n++] = ship_slot_index(shm, s);
        }
        /* потом обычные */
        while (n < max_size) {
            ShipDesc *nu = shm_ships_peek_normal(shm, SHIP_UP);
            ShipDesc *nd = shm_ships_peek_normal(shm, SHIP_DOWN);
            if (!nu && !nd) break;
            ShipDirection dir;
            if (nu && (!nd || nu->arrival_time <= nd->arrival_time))
                dir = SHIP_UP;
            else
                dir = SHIP_DOWN;
            ShipDesc *s = shm_ships_pop_normal(shm, dir);
            if (!s) break;
            s->group_id = (int)n;
            s->state    = SHIP_IN_GROUP;
            shm->ship_group[n++] = ship_slot_index(shm, s);
        }
    } else {
        /* FIFO: строго по arrival_time */
        while (n < max_size) {
            ShipDesc *pu = shm_ships_peek_priority(shm, SHIP_UP);
            ShipDesc *pd = shm_ships_peek_priority(shm, SHIP_DOWN);
            ShipDesc *nu = shm_ships_peek_normal  (shm, SHIP_UP);
            ShipDesc *nd = shm_ships_peek_normal  (shm, SHIP_DOWN);

            ShipDesc *best = NULL;
            ShipDirection best_dir = SHIP_UP;
            int best_prio = 0;

            if (pu) { best = pu; best_dir = SHIP_UP;   best_prio = 1; }
            if (pd && (!best || pd->arrival_time < best->arrival_time)) {
                best = pd; best_dir = SHIP_DOWN; best_prio = 1;
            }
            if (nu && (!best || nu->arrival_time < best->arrival_time)) {
                best = nu; best_dir = SHIP_UP;   best_prio = 0;
            }
            if (nd && (!best || nd->arrival_time < best->arrival_time)) {
                best = nd; best_dir = SHIP_DOWN; best_prio = 0;
            }
            if (!best) break;

            ShipDesc *s = best_prio
                ? shm_ships_pop_priority(shm, best_dir)
                : shm_ships_pop_normal  (shm, best_dir);
            if (!s) break;
            s->group_id = (int)n;
            s->state    = SHIP_IN_GROUP;
            shm->ship_group[n++] = ship_slot_index(shm, s);
        }
    }

    shm->bridge.next_in_group = 0;
    shm->ship_group_size = (int)n;
}

bool operator_review(SharedState *shm) {
    Bridge *b = &shm->bridge;

    switch (b->state) {

    case ROAD_OPEN: {
        size_t n_ships = shm_ships_size(shm, SHIP_UP)
                       + shm_ships_size(shm, SHIP_DOWN);
        if (n_ships > 0) {
            bridge_begin_close_barrier(b, 0.0);
            shm->mechanism_cmd = MECH_CLOSE_BARRIER;
            return true;
        }
        break;
    }

    case WAIT_ROAD_CLEAR:
        if (bridge_can_start_raise(b)) {
            bridge_begin_raise(b, 0.0);
            shm->mechanism_cmd = MECH_RAISE;
            return true;
        }
        break;

    case SHIPS_PASSING:
        if (shm->ship_group_size == 0) {
            operator_form_ship_group(shm);
            if (shm->ship_group_size == 0) {
                bridge_begin_lower(b, 0.0);
                shm->mechanism_cmd = MECH_LOWER;
                return true;
            }
            b->ships_passing = shm->ship_group_size;
            return false;
        }
        if (bridge_can_start_lower(b)) {
            bridge_begin_lower(b, 0.0);
            shm->mechanism_cmd = MECH_LOWER;
            shm->ship_group_size = 0;
            return true;
        }
        break;

    case LOWER:
        if (b->span == LOWERED) {
            bridge_begin_open_barrier(b, 0.0);
            shm->mechanism_cmd = MECH_OPEN_BARRIER;
            return true;
        }
        break;

    default:
        break;
    }
    return false;
}