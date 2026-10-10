#ifndef EVENT_H
#define EVENT_H

#include <stdbool.h>
#include <stddef.h>
#include "states.h"

typedef enum {
    EV_CAR_ARRIVAL = 0,
    EV_PRIORITY_CAR_ARRIVAL,
    EV_SHIP_ARRIVAL,
    EV_PRIORITY_SHIP_ARRIVAL,

    EV_BARRIER_CLOSE_DONE,
    EV_RAISE_DONE,
    EV_SHIP_ENTER,
    EV_SHIP_EXIT,
    EV_LOWER_DONE,
    EV_BARRIER_OPEN_DONE,

    EV_CAR_ENTER,
    EV_CAR_EXIT,

    EV_SHIFT_END,
    EV_TYPE_COUNT
} EventType;

typedef struct {
    Time           time;
    EventType      type;
    Id             entity_id;
    ShipDirection  direction;
    Priority       priority;
    void          *payload;
} Event;

typedef struct {
    Event *data;
    size_t size;
    size_t capacity;
} EventQueue;

bool   event_queue_init(EventQueue *q, size_t capacity);
void   event_queue_free(EventQueue *q);

bool   event_queue_push(EventQueue *q, Event ev);
bool   event_queue_pop_min(EventQueue *q, Event *out);
bool   event_queue_peek_min(const EventQueue *q, Event *out);

bool   event_queue_empty(const EventQueue *q);
size_t event_queue_size(const EventQueue *q);

const char *event_type_name(EventType t);

#endif /* EVENT_H */
