#include "config.h"
#include "ipc.h"
#include "bridge.h"
#include "logger.h"
#include "signals.h"
#include "time_util.h"
#include "processes.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void shift_timer_main(SharedState *shm, double shift_end) {
    sleep_seconds(shift_end);
    if (shm->stop) return;
    mutex_lock();
    shm->stop = 1;
    mutex_unlock();
    sem_post(g_event);
    sem_post(g_mech);
}

static void max_served_watchdog(SharedState *shm, int max_served) {
    if (max_served <= 0) return;
    while (!shm->stop) {
        mutex_lock();
        int done = (int)shm->served_total >= max_served;
        mutex_unlock();
        if (done) {
            mutex_lock();
            shm->stop = 1;
            mutex_unlock();
            sem_post(g_event);
            sem_post(g_mech);
            return;
        }
        sleep_seconds(0.5);
    }
}

int main(int argc, char **argv) {
    Config cfg;
    config_defaults(&cfg);
    if (!config_parse_args(&cfg, argc, argv)) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--help") == 0) return 0;
        }
        return 1;
    }
    config_print(&cfg);

    {
        int fd = open("sim.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
    }

    if (setpgid(0, 0) < 0) {
        perror("setpgid");
    }

    SharedState *shm = ipc_setup();
    if (!shm) return 1;

    shm->cfg = cfg;

    BridgeTiming t = {
        .barrier_close_time = cfg.barrier_close_time,
        .barrier_open_time  = cfg.barrier_open_time,
        .raise_time         = cfg.raise_time,
        .lower_time         = cfg.lower_time,
    };
    bridge_init(&shm->bridge, &t);

    ring_init(&shm->cars_normal,         cfg.car_queue_cap);
    ring_init(&shm->cars_priority,       cfg.car_queue_cap);
    ring_init(&shm->ships_up_normal,     cfg.ship_queue_cap);
    ring_init(&shm->ships_up_priority,   cfg.ship_queue_cap);
    ring_init(&shm->ships_down_normal,   cfg.ship_queue_cap);
    ring_init(&shm->ships_down_priority, cfg.ship_queue_cap);

    shm->next_car_id   = 1;
    shm->next_ship_id  = 1;
    shm->mechanism_cmd = MECH_IDLE;

    sim_time_init();
    signals_install(&shm->stop);

    if (fork() == 0) { mechanism_process_main(shm);           exit(0); }
    if (fork() == 0) { operator_process_main (shm);           exit(0); }
    if (fork() == 0) { car_generator_main    (shm);           exit(0); }
    if (fork() == 0) { ship_generator_main   (shm, SHIP_UP);  exit(0); }
    if (fork() == 0) { ship_generator_main   (shm, SHIP_DOWN);exit(0); }

    if (cfg.shift_end > 0.0) {
        if (fork() == 0) { shift_timer_main(shm, cfg.shift_end); exit(0); }
    }
    if (cfg.max_served > 0) {
        if (fork() == 0) { max_served_watchdog(shm, cfg.max_served); exit(0); }
    }

    while (!shm->stop) sleep_seconds(0.2);

    sem_post(g_event);
    sem_post(g_mech);

    kill(0, SIGTERM);

    int status;
    while (1) {
        pid_t w = waitpid(-1, &status, 0);
        if (w > 0) continue;
        if (w < 0 && errno == EINTR) continue;
        break;
    }

    Logger lg;
    logger_init(&lg, "sim.log", 0.0, g_log_mutex);
    logger_summary(&lg, sim_time_now(), shm);
    logger_close(&lg);

    ipc_cleanup();
    return 0;
}