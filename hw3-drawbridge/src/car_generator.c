#include "processes.h"
#include "ipc.h"
#include "rng.h"
#include "logger.h"
#include "time_util.h"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <semaphore.h>

static int alloc_car_slot(SharedState *shm) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (!shm->car_slots[i].used) return i;
    }
    return -1;
}

void car_generator_main(SharedState *shm) {
    Logger lg;
    logger_init(&lg, "sim.log", shm->cfg.display_delay, g_log_mutex);

    Rng rng;
    rng_init(&rng, shm->cfg.seed ^ 0xC0FFEEu);

    while (!shm->stop) {
        double dt = rng_exp_interval(&rng, shm->cfg.car_lambda);
        bool prio = rng_chance(&rng, shm->cfg.priority_prob_car);

        sleep_seconds(dt);
        if (shm->stop) break;

        mutex_lock();
        int slot = alloc_car_slot(shm);
        if (slot < 0) {
            shm->cars_rejected++;
            mutex_unlock();
            logger_printf(&lg, "[t=%7.2f] CAR_REJECTED (no slot)\n",
                          sim_time_now());
            continue;
        }
        int my_id = shm->next_car_id++;
        shm->car_slots[slot].id           = my_id;
        shm->car_slots[slot].priority     = prio ? EMERGENCY : NORMAL;
        shm->car_slots[slot].state        = CAR_QUEUED;
        shm->car_slots[slot].arrival_time = sim_time_now();
        shm->car_slots[slot].used         = 1;
        mutex_unlock();

        logger_printf(&lg, "[t=%7.2f] CAR_ARRIVAL id=%d%s\n",
                      sim_time_now(), my_id,
                      prio ? " (priority)" : "");

        pid_t pid = fork();
        if (pid == 0) {
            car_process_main(shm, slot);
            exit(0);
        }
        if (pid < 0) {
            mutex_lock();
            shm->car_slots[slot].used = 0;
            mutex_unlock();
        }

        sem_post(g_event);

        /* подбираем зомби, чтобы не копились */
        while (waitpid(-1, NULL, WNOHANG) > 0) ;
    }

    while (1) {
        pid_t w = waitpid(-1, NULL, 0);
        if (w > 0) continue;
        if (w < 0 && errno == EINTR) continue;
        break;
    }
}