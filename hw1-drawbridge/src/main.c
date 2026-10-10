#include "config.h"
#include "sim.h"
#include "signals.h"
#include <stdio.h>

int main(int argc, char **argv) {
    Config cfg;
    config_defaults(&cfg);

    if (!config_parse_args(&cfg, argc, argv)) {
        return 1;
    }

    config_print(&cfg);

    signals_install(&g_stop_requested);

    Simulation sim;
    if (!sim_init(&sim, &cfg)) {
        fprintf(stderr, "sim_init failed\n");
        return 1;
    }

    int rc = sim_run(&sim);
    sim_free(&sim);
    return rc;
}
