#include "operator.h"

void operator_init(Operator *op, GroupStrategy strat) {
    if (!op) return;

    op->strategy = strat;
    op->current_group_id   = 0;
    op->current_group_size = 0;
}

bool operator_review(Operator *op,
                     Bridge *bridge,
                     const Stream *cars,
                     const Stream *ships_up,
                     const Stream *ships_down,
                     Time now)
{
    (void) cars;   /* решения оператора пока не зависят от очереди машин */

    if (!op || !bridge) return false;

    switch (bridge->state) {

    case ROAD_OPEN: {
        size_t n_ships = stream_size(ships_up) + stream_size(ships_down);

        if (n_ships > 0) {
            bridge_begin_close_barrier(bridge, now);
            return true;
        }
        break;
    }

    case WAIT_ROAD_CLEAR: {
        if (bridge_can_start_raise(bridge)) {
            bridge_begin_raise(bridge, now);
            return true;
        }
        break;
    }

    case SHIPS_PASSING: {
        if (bridge_can_start_lower(bridge)) {
            bridge_begin_lower(bridge, now);
            return true;
        }
        break;
    }

    case LOWER: {
        if (bridge->span == LOWERED) {
            bridge_begin_open_barrier(bridge, now);
            return true;
        }
        break;
    }

    default:
        break;
    }

    return false;
}

static size_t form_group_fifo(Stream *s_up, Stream *s_down,
                              Ship **out, size_t max_size)
{
    size_t n = 0;

    while (n < max_size) {
        const Ship *pu = (const Ship *) stream_peek_priority(s_up);
        const Ship *nu = (const Ship *) stream_peek_normal  (s_up);
        const Ship *pd = (const Ship *) stream_peek_priority(s_down);
        const Ship *nd = (const Ship *) stream_peek_normal  (s_down);

        const Ship *best        = NULL;
        Stream     *best_stream = NULL;
        int         from_priority = 0;

        if (pu) {
            best = pu; best_stream = s_up;   from_priority = 1;
        }
        if (pd && (!best || pd->arrival_time < best->arrival_time)) {
            best = pd; best_stream = s_down; from_priority = 1;
        }
        if (nu && (!best || nu->arrival_time < best->arrival_time)) {
            best = nu; best_stream = s_up;   from_priority = 0;
        }
        if (nd && (!best || nd->arrival_time < best->arrival_time)) {
            best = nd; best_stream = s_down; from_priority = 0;
        }

        if (!best) break;

        out[n++] = from_priority
                 ? (Ship *) stream_pop_priority(best_stream)
                 : (Ship *) stream_pop_normal  (best_stream);
    }
    return n;
}

static size_t form_group_priority(Stream *s_up, Stream *s_down,
                                  Ship **out, size_t max_size)
{
    size_t n = 0;

    while (n < max_size) {
        const Ship *pu = (const Ship *) stream_peek_priority(s_up);
        const Ship *pd = (const Ship *) stream_peek_priority(s_down);

        if (!pu && !pd) break;

        if (pu && (!pd || pu->arrival_time <= pd->arrival_time)) {
            out[n++] = (Ship *) stream_pop_priority(s_up);
        } else {
            out[n++] = (Ship *) stream_pop_priority(s_down);
        }
    }

    while (n < max_size) {
        const Ship *nu = (const Ship *) stream_peek_normal(s_up);
        const Ship *nd = (const Ship *) stream_peek_normal(s_down);

        if (!nu && !nd) break;

        if (nu && (!nd || nu->arrival_time <= nd->arrival_time)) {
            out[n++] = (Ship *) stream_pop_normal(s_up);
        } else {
            out[n++] = (Ship *) stream_pop_normal(s_down);
        }
    }

    return n;
}

size_t operator_form_ship_group(Operator *op,
                                Stream *ships_up,
                                Stream *ships_down,
                                Ship **out_ships,
                                size_t max_size)
{
    if (!op || !ships_up || !ships_down || !out_ships || max_size == 0)
        return 0;

    size_t n;
    if (op->strategy == GROUP_STRATEGY_PRIORITY) {
        n = form_group_priority(ships_up, ships_down, out_ships, max_size);
    } else {
        n = form_group_fifo(ships_up, ships_down, out_ships, max_size);
    }

    for (size_t i = 0; i < n; ++i) {
        out_ships[i]->group_id = op->current_group_id;
    }
    op->current_group_id++;
    op->current_group_size = (int) n;
    return n;
}

void operator_notify_timeout(Operator *op, Id entity_id) {
    (void) op;
    (void) entity_id;
}
