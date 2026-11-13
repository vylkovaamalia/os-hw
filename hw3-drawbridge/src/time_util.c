#include "time_util.h"
#include <errno.h>
#include <time.h>

static struct timespec g_start;

void sim_time_init(void) {
    clock_gettime(CLOCK_MONOTONIC, &g_start);
}

double sim_time_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)(ts.tv_sec  - g_start.tv_sec)
         + (double)(ts.tv_nsec - g_start.tv_nsec) / 1e9;
}

void sleep_seconds(double s) {
    if (s <= 0.0) return;
    struct timespec ts = {
        .tv_sec  = (time_t)s,
        .tv_nsec = (long)((s - (time_t)s) * 1e9)
    };
    nanosleep(&ts, NULL);
}