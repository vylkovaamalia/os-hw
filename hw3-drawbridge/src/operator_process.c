#include "processes.h"
#include "operator.h"
#include "ipc.h"
#include "bridge.h"
#include "logger.h"
#include "time_util.h"

#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>

void operator_process_main(SharedState *shm) {
    Logger lg;
    logger_init(&lg, "sim.log", shm->cfg.display_delay, g_log_mutex);

    while (1) {
        while (sem_wait(g_event) < 0) {
            if (errno == EINTR) {
                if (shm->stop) return;
                continue;
            }
            return;
        }
        if (shm->stop) return;

        mutex_lock();
        BridgeState before = shm->bridge.state;
        bool acted = operator_review(shm);
        BridgeState after = shm->bridge.state;
        if (acted) {
            logger_printf(&lg, "[t=%7.2f] OP: %s -> %s, cmd=%d\n",
                          sim_time_now(),
                          bridge_state_name(before),
                          bridge_state_name(after),
                          (int)shm->mechanism_cmd);
        }
        mutex_unlock();

        if (acted) sem_post(g_mech);
    }
}