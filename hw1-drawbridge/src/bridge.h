#ifndef BRIDGE_H
#define BRIDGE_H

#include <stdbool.h>
#include "states.h"

typedef struct {
    double barrier_close_time;
    double barrier_open_time;
    double raise_time;
    double lower_time;
} BridgeTiming;

typedef struct {
    BridgeState  state;
    BarrierState barrier;
    SpanState    span;
    Permission   permission;

    int          cars_on_bridge;
    int          ships_passing;

    Time         state_enter_time;
    Time         state_end_time;

    BridgeTiming timing;
} Bridge;

void bridge_init(Bridge *b, const BridgeTiming *timing);

bool bridge_check_invariants(const Bridge *b);

bool bridge_can_start_raise(const Bridge *b);
bool bridge_can_start_lower(const Bridge *b);

void bridge_begin_close_barrier(Bridge *b, Time now);
void bridge_begin_open_barrier (Bridge *b, Time now);
void bridge_begin_raise        (Bridge *b, Time now);
void bridge_begin_lower        (Bridge *b, Time now);

void bridge_finish_operation(Bridge *b, Time now);

const char *bridge_state_name(BridgeState s);

#endif /* BRIDGE_H */
