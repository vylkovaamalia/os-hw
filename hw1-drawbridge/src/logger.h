#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include "states.h"
#include "event.h"
#include "bridge.h"
#include "stream.h"

typedef struct {
    int    fd_out;
    int    fd_log;
    double delay;
} Logger;

void logger_init(Logger *lg, const char *path, double delay);

void logger_close(Logger *lg);

void logger_printf(Logger *lg, const char *fmt, ...);

// [t=...] EVENT: NAME ...
void logger_event(Logger *lg, Time now, const Event *ev);

void logger_bridge(Logger *lg, Time now, const Bridge *b);

void logger_queues(Logger *lg, Time now,
                   const Stream *cars,
                   const Stream *ships_up,
                   const Stream *ships_down);

void logger_summary(Logger *lg,
                    Time now,
                    unsigned cars_served,
                    unsigned ships_served,
                    unsigned cars_rejected,
                    unsigned ships_rejected,
                    size_t   cars_left,
                    size_t   ships_up_left,
                    size_t   ships_down_left);

#endif /* LOGGER_H */
