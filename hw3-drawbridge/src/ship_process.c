#include "processes.h"
#include "ipc.h"
#include "bridge.h"
#include "logger.h"
#include "time_util.h"

#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>

#define POLL_INTERVAL 0.05

void ship_process_main(SharedState *shm, int slot_idx) {
    Logger lg;
    logger_init(&lg, "sim.log", shm->cfg.display_delay, g_log_mutex);

    ShipDesc *me = &shm->ship_slots[slot_idx];

    int entered = 0;
    while (!shm->stop) {
        int can = 0;
        mutex_lock();
        if (shm->bridge.state == SHIPS_PASSING &&
            me->state == SHIP_IN_GROUP &&
            me->group_id == shm->bridge.next_in_group) {
            me->state = SHIP_PASSING;
            shm->bridge.next_in_group++;
            can = 1;
            logger_printf(&lg, "[t=%7.2f] SHIP_ENTER id=%d dir=%s\n",
                          sim_time_now(), me->id,
                          me->direction == SHIP_UP ? "up" : "down");
        }
        mutex_unlock();

        if (can) { entered = 1; break; }
        sleep_seconds(POLL_INTERVAL);
    }

    if (!entered) {
        mutex_lock();
        if (me->state == SHIP_IN_GROUP) {
            shm->bridge.ships_passing--;
        }
        me->used = 0;
        mutex_unlock();
        return;
    }

    sleep_seconds(shm->cfg.ship_pass_time);

    mutex_lock();
    me->state = SHIP_DONE;
    me->used  = 0;
    shm->bridge.ships_passing--;
    shm->ships_served++;
    shm->served_total++;
    logger_printf(&lg, "[t=%7.2f] SHIP_EXIT  id=%d dir=%s\n",
                  sim_time_now(), me->id,
                  me->direction == SHIP_UP ? "up" : "down");
    mutex_unlock();

    sem_post(g_event);
}