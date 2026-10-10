#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "states.h"

typedef struct {
    Id          id;
    Priority    priority;
    CarState    state;

    Time        arrival_time;
    Time        bridge_enter_time;
    Time        finish_time;

    double      max_wait;
    bool        timeout;
} Car;

typedef struct {
    Id            id;
    ShipDirection direction;
    Priority      priority;
    ShipState     state;

    Time          arrival_time;
    Time          pass_start_time;
    Time          finish_time;

    double        max_wait;
    bool          timeout;

    int           group_id;
} Ship;

Car  *car_create(Id id, Priority priority, Time arrival, double max_wait);
void  car_destroy(Car *car);

Ship *ship_create(Id id, ShipDirection dir, Priority priority,
                  Time arrival, double max_wait);
void  ship_destroy(Ship *ship);

#endif /* TRANSPORT_H */
