#include "event.h"
#include <stdlib.h>

static void swap(Event *a, Event *b) {
    Event tmp = *a;
    *a = *b;
    *b = tmp;
}

static void sift_up(EventQueue *q, size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (q->data[parent].time <= q->data[index].time) break;
        swap(&q->data[parent], &q->data[index]);
        index = parent;
    }
}

static void sift_down(EventQueue *q, size_t index) {
    for (;;) {
        size_t left  = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t smallest = index;

        if (left  < q->size && q->data[left].time  < q->data[smallest].time)
            smallest = left;
        if (right < q->size && q->data[right].time < q->data[smallest].time)
            smallest = right;

        if (smallest == index) break;
        swap(&q->data[index], &q->data[smallest]);
        index = smallest;
    }
}

bool event_queue_init(EventQueue *q, size_t capacity) {
    if (!q || capacity == 0) return false;

    q->data = malloc(capacity * sizeof(Event));
    if (!q->data) return false;

    q->size     = 0;
    q->capacity = capacity;
    return true;
}

void event_queue_free(EventQueue *q) {
    if (!q) return;
    free(q->data);
    q->data     = NULL;
    q->size     = 0;
    q->capacity = 0;
}

bool event_queue_push(EventQueue *q, Event ev) {
    if (!q || q->size >= q->capacity)
        return false;

    q->data[q->size] = ev;
    sift_up(q, q->size);
    q->size++;
    return true;
}

bool event_queue_pop_min(EventQueue *q, Event *out) {
    if (!q || q->size == 0)
        return false;

    if (out) *out = q->data[0];

    q->size--;
    if (q->size > 0) {
        q->data[0] = q->data[q->size];
        sift_down(q, 0);
    }
    return true;
}

bool event_queue_peek_min(const EventQueue *q, Event *out) {
    if (!q || q->size == 0)
        return false;

    if (out) *out = q->data[0];
    return true;
}

bool event_queue_empty(const EventQueue *q) {
    return !q || q->size == 0;
}

size_t event_queue_size(const EventQueue *q) {
    return q ? q->size : 0;
}

const char *event_type_name(EventType t) {
    switch (t) {
    case EV_CAR_ARRIVAL:            return "CAR_ARRIVAL";
    case EV_PRIORITY_CAR_ARRIVAL:   return "PRIORITY_CAR_ARRIVAL";
    case EV_SHIP_ARRIVAL:           return "SHIP_ARRIVAL";
    case EV_PRIORITY_SHIP_ARRIVAL:  return "PRIORITY_SHIP_ARRIVAL";

    case EV_BARRIER_CLOSE_DONE:     return "BARRIER_CLOSE_DONE";
    case EV_RAISE_DONE:             return "RAISE_DONE";
    case EV_SHIP_ENTER:             return "SHIP_ENTER";
    case EV_SHIP_EXIT:              return "SHIP_EXIT";
    case EV_LOWER_DONE:             return "LOWER_DONE";
    case EV_BARRIER_OPEN_DONE:      return "BARRIER_OPEN_DONE";

    case EV_CAR_ENTER:              return "CAR_ENTER";
    case EV_CAR_EXIT:               return "CAR_EXIT";

    case EV_SHIFT_END:              return "SHIFT_END";
    default:                        return "?";
    }
}
