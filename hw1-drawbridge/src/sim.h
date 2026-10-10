#ifndef SIM_H
#define SIM_H

#include <stdbool.h>
#include <signal.h>
#include "config.h"
#include "bridge.h"
#include "operator.h"
#include "stream.h"
#include "event.h"
#include "rng.h"
#include "transport.h"
#include "logger.h"

#define SIM_MAX_GROUP 64

typedef struct {
    Config     cfg;
    Rng        rng;
    Bridge     bridge;
    Operator   op;

    Stream     cars;
    Stream     ships_up;
    Stream     ships_down;

    EventQueue events;

    Time       now;
    Id         next_car_id;
    Id         next_ship_id;

    unsigned   served_total;
    unsigned   cars_served;
    unsigned   ships_served;
    unsigned   cars_rejected;
    unsigned   ships_rejected;

    Logger     logger;
} Simulation;

bool sim_init(Simulation *sim, const Config *cfg);
void sim_free(Simulation *sim);
int  sim_run (Simulation *sim);

extern volatile sig_atomic_t g_stop_requested;

#endif /* SIM_H */
