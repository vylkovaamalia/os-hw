#include "transport.h"
#include <stdlib.h>

Car *car_create(Id id, Priority priority, Time arrival, double max_wait) {
    Car *c = malloc(sizeof(Car));
    if (!c) return NULL;

    c->id                = id;
    c->priority          = priority;
    c->state             = CAR_QUEUED;

    c->arrival_time      = arrival;
    c->bridge_enter_time = 0.0;
    c->finish_time       = 0.0;

    c->max_wait          = max_wait;
    c->timeout           = false;

    return c;
}

void car_destroy(Car *car) {
    free(car);
}


Ship *ship_create(Id id, ShipDirection dir, Priority priority,
                  Time arrival, double max_wait) {
    Ship *s = malloc(sizeof(Ship));
    if (!s) return NULL;

    s->id               = id;
    s->direction        = dir;
    s->priority         = priority;
    s->state            = SHIP_QUEUED;

    s->arrival_time     = arrival;
    s->pass_start_time  = 0.0;
    s->finish_time      = 0.0;

    s->max_wait         = max_wait;
    s->timeout          = false;

    s->group_id         = -1;

    return s;
}

void ship_destroy(Ship *ship) {
    free(ship);
}
