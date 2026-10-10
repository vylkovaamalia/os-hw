#include "bridge.h"

// Инициализация

void bridge_init(Bridge *b, const BridgeTiming *timing) {
    if (!b) return;

    b->state           = ROAD_OPEN;
    b->barrier         = BARRIER_OPEN;
    b->span            = LOWERED;
    b->permission      = CARS;

    b->cars_on_bridge  = 0;
    b->ships_passing   = 0;

    b->state_enter_time = 0.0;
    b->state_end_time   = 0.0;

    if (timing) {
        b->timing = *timing;
    } else {
        b->timing.barrier_close_time = 0.0;
        b->timing.barrier_open_time  = 0.0;
        b->timing.raise_time         = 0.0;
        b->timing.lower_time         = 0.0;
    }
}

// Инварианты

bool bridge_check_invariants(const Bridge *b) {
    if (!b) return false;

    if (b->cars_on_bridge < 0) return false;
    if (b->ships_passing  < 0) return false;

    switch (b->state) {
    case ROAD_OPEN:
        if (b->permission != CARS)           return false;
        if (b->barrier    != BARRIER_OPEN)   return false;
        if (b->span       != LOWERED)        return false;
        break;

    case CLOSE_BARRIER:
        if (b->permission != NONE)           return false;
        if (b->barrier    != BARRIER_CLOSED) return false;
        if (b->span       != LOWERED)        return false;
        break;

    case WAIT_ROAD_CLEAR:
        if (b->permission != NONE)           return false;
        if (b->barrier    != BARRIER_CLOSED) return false;
        if (b->span       != LOWERED)        return false;
        break;

    case RAISE:
        if (b->permission != NONE)           return false;
        if (b->cars_on_bridge != 0)          return false;
        if (b->barrier    != BARRIER_CLOSED) return false;
        if (b->span       != MOVING)         return false;
        break;

    case SHIPS_PASSING:
        if (b->permission != SHIPS)          return false;
        if (b->span       != RAISED)         return false;
        if (b->barrier    != BARRIER_CLOSED) return false;
        if (b->cars_on_bridge != 0)          return false;
        break;

    case LOWER:
        if (b->permission   != NONE)                 return false;
        if (b->ships_passing != 0)                   return false;
        if (b->cars_on_bridge != 0)                  return false;
        if (b->span != MOVING && b->span != LOWERED) return false;
        break;

    case OPEN_BARRIER:
        if (b->permission != NONE)           return false;
        if (b->barrier    != BARRIER_OPEN)   return false;
        if (b->span       != LOWERED)        return false;
        if (b->ships_passing != 0)           return false;
        break;

    default:
        return false;
    }

    return true;
}

// Проверки допустимости переходов

bool bridge_can_start_raise(const Bridge *b) {
    if (!b) return false;
    if (b->state != WAIT_ROAD_CLEAR) return false;
    if (b->cars_on_bridge != 0)      return false;
    return true;
}

bool bridge_can_start_lower(const Bridge *b) {
    if (!b) return false;
    if (b->state != SHIPS_PASSING) return false;
    if (b->ships_passing != 0)    return false;
    return true;
}

// Команды начала операций

void bridge_begin_close_barrier(Bridge *b, Time now) {
    if (!b || b->state != ROAD_OPEN) return;

    b->state           = CLOSE_BARRIER;
    b->barrier         = BARRIER_CLOSED;
    b->permission      = NONE;
    b->state_enter_time = now;
    b->state_end_time   = now + b->timing.barrier_close_time;
}

void bridge_begin_raise(Bridge *b, Time now) {
    if (!b || b->state != WAIT_ROAD_CLEAR) return;
    if (b->cars_on_bridge != 0) return;

    b->state           = RAISE;
    b->span            = MOVING;
    b->permission      = NONE;
    b->state_enter_time = now;
    b->state_end_time   = now + b->timing.raise_time;
}

void bridge_begin_lower(Bridge *b, Time now) {
    if (!b || b->state != SHIPS_PASSING) return;
    if (b->ships_passing != 0) return;

    b->state           = LOWER;
    b->span            = MOVING;
    b->permission      = NONE;
    b->state_enter_time = now;
    b->state_end_time   = now + b->timing.lower_time;
}

void bridge_begin_open_barrier(Bridge *b, Time now) {
    if (!b || b->state != LOWER) return;
    if (b->span != LOWERED) return;

    b->state           = OPEN_BARRIER;
    b->barrier         = BARRIER_OPEN;
    b->state_enter_time = now;
    b->state_end_time   = now + b->timing.barrier_open_time;
}

// Завершение текущей операции

void bridge_finish_operation(Bridge *b, Time now) {
    if (!b) return;

    (void) now;

    switch (b->state) {
    case CLOSE_BARRIER:
        b->state   = WAIT_ROAD_CLEAR;
        break;

    case RAISE:
        b->span       = RAISED;
        b->permission = SHIPS;
        b->state      = SHIPS_PASSING;
        break;

    case LOWER:
        b->span = LOWERED;
        break;

    case OPEN_BARRIER:
        b->barrier    = BARRIER_OPEN;
        b->permission = CARS;
        b->state      = ROAD_OPEN;
        break;

    default:
        break;
    }
}

const char *bridge_state_name(BridgeState s) {
    switch (s) {
    case ROAD_OPEN:       return "ROAD_OPEN";
    case CLOSE_BARRIER:   return "CLOSE_BARRIER";
    case WAIT_ROAD_CLEAR: return "WAIT_ROAD_CLEAR";
    case RAISE:           return "RAISE";
    case SHIPS_PASSING:   return "SHIPS_PASSING";
    case LOWER:           return "LOWER";
    case OPEN_BARRIER:    return "OPEN_BARRIER";
    default:              return "?";
    }
}
