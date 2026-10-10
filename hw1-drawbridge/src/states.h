#ifndef STATES_H
#define STATES_H

#include <stdbool.h>
#include <stddef.h>

typedef int    Id;
typedef double Time;

typedef enum {
    SHIP_UP = 0,
    SHIP_DOWN = 1,
    SHIP_DIR_COUNT
} ShipDirection;

typedef enum {
    NORMAL = 0,
    EMERGENCY = 1
} Priority;

typedef enum {
    CAR_QUEUED,
    CAR_ON_BRIDGE,
    CAR_DONE
} CarState;

typedef enum {
    SHIP_QUEUED,
    SHIP_IN_GROUP,
    SHIP_PASSING,
    SHIP_DONE
} ShipState;

typedef enum {
    ROAD_OPEN = 0,
    CLOSE_BARRIER,
    WAIT_ROAD_CLEAR,
    RAISE,
    SHIPS_PASSING,
    LOWER,
    OPEN_BARRIER,
    BRIDGE_STATE_COUNT
} BridgeState;

typedef enum {
    BARRIER_OPEN = 0,
    BARRIER_CLOSED
} BarrierState;

typedef enum {
    LOWERED = 0,
    RAISED,
    MOVING
} SpanState;

typedef enum {
    NONE = 0,
    CARS,
    SHIPS
} Permission;

typedef enum {
    GROUP_STRATEGY_FIFO = 0,
    GROUP_STRATEGY_PRIORITY
} GroupStrategy;

#endif /* STATES_H */
