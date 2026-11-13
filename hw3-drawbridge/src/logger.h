#ifndef LOGGER_H
#define LOGGER_H

#include <semaphore.h>
#include "states.h"
#include "bridge.h"
#include "shared.h"

typedef struct {
    int     fd_out;
    int     fd_log;
    double  delay;
    sem_t  *log_mutex;
} Logger;

void logger_init (Logger *lg, const char *path, double delay, sem_t *log_mutex);
void logger_close(Logger *lg);

void logger_printf(Logger *lg, const char *fmt, ...);

void logger_bridge (Logger *lg, double now, const Bridge *b);
void logger_queues (Logger *lg, double now, const SharedState *shm);
void logger_summary(Logger *lg, double now, const SharedState *shm);

#endif