#include "processes.h"
#include "ipc.h"
#include "bridge.h"
#include "logger.h"
#include "time_util.h"

#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>

static double cmd_duration(const SharedState *shm, MechCmd cmd) {
    switch (cmd) {
    case MECH_CLOSE_BARRIER: return shm->cfg.barrier_close_time;
    case MECH_RAISE:         return shm->cfg.raise_time;
    case MECH_LOWER:         return shm->cfg.lower_time;
    case MECH_OPEN_BARRIER:  return shm->cfg.barrier_open_time;
    default:                 return 0.0;
    }
}

static const char *cmd_name(MechCmd cmd) {
    switch (cmd) {
    case MECH_CLOSE_BARRIER: return "CLOSE_BARRIER";
    case MECH_RAISE:         return "RAISE";
    case MECH_LOWER:         return "LOWER";
    case MECH_OPEN_BARRIER:  return "OPEN_BARRIER";
    case MECH_STOP:          return "STOP";
    default:                 return "IDLE";
    }
}

void mechanism_process_main(SharedState *shm) {
    Logger lg;
    logger_init(&lg, "sim.log", shm->cfg.display_delay, g_log_mutex);

    while (1) {
        while (sem_wait(g_mech) < 0) {
            if (errno == EINTR) {
                if (shm->stop) return;
                continue;
            }
            return;
        }
        if (shm->stop) return;

        mutex_lock();
        MechCmd cmd = shm->mechanism_cmd;
        shm->mechanism_cmd = MECH_IDLE;
        double duration = cmd_duration(shm, cmd);
        mutex_unlock();

        if (cmd == MECH_IDLE) continue;

        logger_printf(&lg, "[t=%7.2f] MECH: begin %s (%.2fs)\n",
                      sim_time_now(), cmd_name(cmd), duration);

        double left = duration;
        while (left > 0.0 && !shm->stop) {
            double step = (left > 0.1) ? 0.1 : left;
            sleep_seconds(step);
            left -= step;
        }
        if (shm->stop) return;

        mutex_lock();
        bridge_finish_operation(&shm->bridge);
        logger_printf(&lg, "[t=%7.2f] MECH: %s done, state=%s\n",
                      sim_time_now(), cmd_name(cmd),
                      bridge_state_name(shm->bridge.state));
        mutex_unlock();

        sem_post(g_event);
    }
}