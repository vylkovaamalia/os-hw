#include "processes.h"
#include "ipc.h"
#include "bridge.h"
#include "logger.h"
#include "time_util.h"

#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>

#define POLL_INTERVAL 0.05

void car_process_main(SharedState *shm, int slot_idx) {
    Logger lg;
    logger_init(&lg, "sim.log", shm->cfg.display_delay, g_log_mutex);

    CarDesc *me = &shm->car_slots[slot_idx];

    /* ждём пока мост откроется для машин */
    int entered = 0;
    while (!shm->stop) {
        mutex_lock();
        if (shm->bridge.state == ROAD_OPEN &&
            shm->bridge.permission == CARS) {
            shm->bridge.cars_on_bridge++;
            me->state = CAR_ON_BRIDGE;
            entered = 1;
            logger_printf(&lg, "[t=%7.2f] CAR_ENTER id=%d\n",
                          sim_time_now(), me->id);
        }
        mutex_unlock();

        if (entered) break;
        sleep_seconds(POLL_INTERVAL);
    }

    if (!entered) {
        mutex_lock();
        me->used = 0;
        mutex_unlock();
        return;
    }

    /* едем */
    sleep_seconds(shm->cfg.car_cross_time);

    mutex_lock();
    shm->bridge.cars_on_bridge--;
    shm->cars_served++;
    shm->served_total++;
    me->state = CAR_DONE;
    me->used  = 0;
    logger_printf(&lg, "[t=%7.2f] CAR_EXIT  id=%d\n",
                  sim_time_now(), me->id);
    mutex_unlock();

    sem_post(g_event);
}