#include "logger.h"
#include "stream.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

void logger_init(Logger *lg, const char *path, double delay, sem_t *log_mutex) {
    lg->fd_out    = STDOUT_FILENO;
    lg->fd_log    = -1;
    lg->delay     = (delay > 0.0) ? delay : 0.0;
    lg->log_mutex = log_mutex;

    if (path && *path) {
        int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) {
            fprintf(stderr, "logger: open('%s'): %s\n", path, strerror(errno));
        } else {
            lg->fd_log = fd;
        }
    }
}

void logger_close(Logger *lg) {
    if (lg->fd_log >= 0) {
        close(lg->fd_log);
        lg->fd_log = -1;
    }
}

static void write_all(int fd, const char *buf, size_t len) {
    if (fd < 0) return;
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        off += (size_t)n;
    }
}

void logger_printf(Logger *lg, const char *fmt, ...) {
    if (!lg || !fmt) return;

    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return;

    size_t len = (size_t)n;
    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1;
        buf[len - 1] = '\n';
    }

    if (lg->log_mutex) {
        while (sem_wait(lg->log_mutex) < 0 && errno == EINTR) ;
    }
    write_all(lg->fd_out, buf, len);
    if (lg->fd_log >= 0) write_all(lg->fd_log, buf, len);
    if (lg->log_mutex) sem_post(lg->log_mutex);

    if (lg->delay > 0.0) usleep((useconds_t)(lg->delay * 1e6));
}

static const char *barrier_name(BarrierState b) {
    return b == BARRIER_OPEN ? "OPEN" : "CLOSED";
}

static const char *span_name(SpanState s) {
    switch (s) {
    case LOWERED: return "LOWERED";
    case RAISED:  return "RAISED";
    case MOVING:  return "MOVING";
    default:      return "?";
    }
}

static const char *permission_name(Permission p) {
    switch (p) {
    case NONE:  return "NONE";
    case CARS:  return "CARS";
    case SHIPS: return "SHIPS";
    default:    return "?";
    }
}

void logger_bridge(Logger *lg, double now, const Bridge *b) {
    logger_printf(lg,
                  "[t=%7.2f] BRIDGE: state=%-14s barrier=%-6s "
                  "span=%-7s perm=%-5s cars=%d ships=%d\n",
                  now, bridge_state_name(b->state), barrier_name(b->barrier),
                  span_name(b->span), permission_name(b->permission),
                  b->cars_on_bridge, b->ships_passing);
}

void logger_queues(Logger *lg, double now, const SharedState *shm) {
    size_t c  = shm_cars_size(shm);
    size_t cp = shm_cars_size_priority(shm);
    size_t u  = shm_ships_size(shm, SHIP_UP);
    size_t up = shm_ships_size_priority(shm, SHIP_UP);
    size_t d  = shm_ships_size(shm, SHIP_DOWN);
    size_t dp = shm_ships_size_priority(shm, SHIP_DOWN);

    logger_printf(lg,
                  "[t=%7.2f] QUEUES: cars=%zu (prio=%zu), "
                  "ships_up=%zu (prio=%zu), ships_down=%zu (prio=%zu)\n",
                  now, c, cp, u, up, d, dp);
}

void logger_summary(Logger *lg, double now, const SharedState *shm) {
    logger_printf(lg, "\n=== Итог ===\n");
    logger_printf(lg, "Модельное время:        %.2f\n", now);
    logger_printf(lg, "Обслужено:              cars=%u, ships=%u\n",
                  shm->cars_served, shm->ships_served);
    logger_printf(lg, "Отказы (очередь полна): cars=%u, ships=%u\n",
                  shm->cars_rejected, shm->ships_rejected);
    logger_printf(lg, "Осталось в очередях:    cars=%zu, ships_up=%zu, ships_down=%zu\n",
                  shm_cars_size(shm),
                  shm_ships_size(shm, SHIP_UP),
                  shm_ships_size(shm, SHIP_DOWN));
}