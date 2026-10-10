#include "bridge.h"
#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

void logger_init(Logger *lg, const char *path, double delay) {
    if (!lg) return;

    lg->fd_out = STDOUT_FILENO;
    lg->fd_log = -1;
    lg->delay  = (delay > 0.0) ? delay : 0.0;

    if (path && *path) {
        int fd = open(path,
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);
        if (fd < 0) {
            fprintf(stderr,
                    "logger: не удалось открыть '%s': %s\n",
                    path, strerror(errno));
        } else {
            lg->fd_log = fd;
        }
    }
}

void logger_close(Logger *lg) {
    if (!lg) return;
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

    write_all(lg->fd_out, buf, len);
    if (lg->fd_log >= 0) {
        write_all(lg->fd_log, buf, len);
    }

    if (lg->delay > 0.0) {
        usleep((useconds_t)(lg->delay * 1e6));
    }
}


static const char *dir_name(ShipDirection d) {
    return (d == SHIP_UP) ? "up" : "down";
}

void logger_event(Logger *lg, Time now, const Event *ev) {
    if (!lg || !ev) return;

    const char *name = event_type_name(ev->type);
    char extra[64] = "";

    switch (ev->type) {
    case EV_SHIP_ARRIVAL:
    case EV_PRIORITY_SHIP_ARRIVAL:
    case EV_SHIP_ENTER:
    case EV_SHIP_EXIT:
        if (ev->entity_id > 0) {
            snprintf(extra, sizeof(extra), " id=%d dir=%s",
                     ev->entity_id, dir_name(ev->direction));
        } else {
            snprintf(extra, sizeof(extra), " dir=%s",
                     dir_name(ev->direction));
        }
        break;

    case EV_CAR_ARRIVAL:
    case EV_PRIORITY_CAR_ARRIVAL:
    case EV_CAR_ENTER:
    case EV_CAR_EXIT:
        if (ev->entity_id > 0) {
            snprintf(extra, sizeof(extra), " id=%d", ev->entity_id);
        }
        break;

    default:
        break;
    }

    logger_printf(lg, "[t=%7.2f] EVENT: %-22s%s\n", now, name, extra);
}

const char *bridge_state_name(BridgeState s);   /* из bridge.h */

static const char *barrier_name(BarrierState b) {
    return (b == BARRIER_OPEN) ? "OPEN" : "CLOSED";
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

void logger_bridge(Logger *lg, Time now, const Bridge *b) {
    if (!lg || !b) return;

    logger_printf(lg,
                  "[t=%7.2f] BRIDGE: state=%-14s barrier=%-6s "
                  "span=%-7s perm=%-5s cars=%d ships=%d\n",
                  now,
                  bridge_state_name(b->state),
                  barrier_name(b->barrier),
                  span_name(b->span),
                  permission_name(b->permission),
                  b->cars_on_bridge,
                  b->ships_passing);
}

void logger_queues(Logger *lg, Time now,
                   const Stream *cars,
                   const Stream *ships_up,
                   const Stream *ships_down)
{
    if (!lg) return;

    size_t c  = cars        ? stream_size(cars)        : 0;
    size_t cp = cars        ? stream_size_priority(cars) : 0;
    size_t u  = ships_up    ? stream_size(ships_up)    : 0;
    size_t up = ships_up    ? stream_size_priority(ships_up) : 0;
    size_t d  = ships_down  ? stream_size(ships_down)  : 0;
    size_t dp = ships_down  ? stream_size_priority(ships_down) : 0;

    logger_printf(lg,
                  "[t=%7.2f] QUEUES: cars=%zu (prio=%zu), "
                  "ships_up=%zu (prio=%zu), ships_down=%zu (prio=%zu)\n",
                  now, c, cp, u, up, d, dp);
}

void logger_summary(Logger *lg,
                    Time now,
                    unsigned cars_served,
                    unsigned ships_served,
                    unsigned cars_rejected,
                    unsigned ships_rejected,
                    size_t   cars_left,
                    size_t   ships_up_left,
                    size_t   ships_down_left)
{
    if (!lg) return;

    logger_printf(lg, "\n=== Итог ===\n");
    logger_printf(lg, "Модельное время:        %.2f\n", now);
    logger_printf(lg, "Обслужено:              cars=%u, ships=%u\n",
                  cars_served, ships_served);
    logger_printf(lg, "Отказы (очередь полна): cars=%u, ships=%u\n",
                  cars_rejected, ships_rejected);
    logger_printf(lg, "Осталось в очередях:    cars=%zu, ships_up=%zu, ships_down=%zu\n",
                  cars_left, ships_up_left, ships_down_left);
}
