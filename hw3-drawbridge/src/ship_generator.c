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

static int alloc_ship_slot(SharedState *shm) {
    for (int i = 0; i < MAX_SHIPS; i++) {
        if (!shm->ship_slots[i].used) return i;
    }
    return -1;
}

void ship_generator_main(SharedState *shm, ShipDirection dir) {
    Logger lg;
    logger_init(&lg, "sim.log", shm->cfg.display_delay, g_log_mutex);

    Rng rng;
    uint64_t salt = (dir == SHIP_UP) ? 0xAB1u : 0xAB2u;
    rng_init(&rng, shm->cfg.seed ^ salt);

    double lambda = (dir == SHIP_UP) ? shm->cfg.ship_lambda_up
                                     : shm->cfg.ship_lambda_down;

    while (!shm->stop) {
        double dt = rng_exp_interval(&rng, lambda);
        bool prio = rng_chance(&rng, shm->cfg.priority_prob_ship);

        sleep_seconds(dt);
        if (shm->stop) break;

        /* сразу форкаем, а слот занимаем только если форк удался;
           так проще не оставить мусор в ring-очереди */
        pid_t pid = fork();
        if (pid < 0) {
            mutex_lock();
            shm->ships_rejected++;
            mutex_unlock();
            logger_printf(&lg, "[t=%7.2f] SHIP_REJECTED (fork) dir=%s\n",
                          sim_time_now(), dir == SHIP_UP ? "up" : "down");
            continue;
        }

        if (pid == 0) {
            /* ребёнку нужен свой слот - занимаем здесь, но уже без гонки,
               потому что родитель не пошёл дальше */
            mutex_lock();
            int slot = alloc_ship_slot(shm);
            if (slot < 0) {
                shm->ships_rejected++;
                mutex_unlock();
                exit(0);
            }
            int my_id = shm->next_ship_id++;
            ShipDesc *s = &shm->ship_slots[slot];
            s->id           = my_id;
            s->direction    = dir;
            s->priority     = prio ? EMERGENCY : NORMAL;
            s->state        = SHIP_QUEUED;
            s->arrival_time = sim_time_now();
            s->group_id     = -1;
            s->used         = 1;

            int *idx_arr;
            RingHeader *ring;
            if (dir == SHIP_UP) {
                if (prio) { idx_arr = shm->ships_up_priority_idx;
                            ring    = &shm->ships_up_priority; }
                else      { idx_arr = shm->ships_up_normal_idx;
                            ring    = &shm->ships_up_normal; }
            } else {
                if (prio) { idx_arr = shm->ships_down_priority_idx;
                            ring    = &shm->ships_down_priority; }
                else      { idx_arr = shm->ships_down_normal_idx;
                            ring    = &shm->ships_down_normal; }
            }
            if (ring_push(ring, idx_arr, slot) < 0) {
                shm->ships_rejected++;
                s->used = 0;
                mutex_unlock();
                exit(0);
            }
            mutex_unlock();

            logger_printf(&lg, "[t=%7.2f] SHIP_ARRIVAL id=%d dir=%s%s\n",
                          sim_time_now(), my_id,
                          dir == SHIP_UP ? "up" : "down",
                          prio ? " (priority)" : "");

            sem_post(g_event);

            ship_process_main(shm, slot);
            exit(0);
        }

        /* подбираем зомби */
        while (waitpid(-1, NULL, WNOHANG) > 0) ;
    }

    while (1) {
        pid_t w = waitpid(-1, NULL, 0);
        if (w > 0) continue;
        if (w < 0 && errno == EINTR) continue;
        break;
    }
}