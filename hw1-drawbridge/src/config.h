#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include "states.h"

typedef struct {
    double car_lambda;
    double ship_lambda_up;
    double ship_lambda_down;

    size_t car_queue_cap;
    size_t ship_queue_cap;

    double car_cross_time;
    double ship_pass_time;
    double barrier_close_time;
    double barrier_open_time;
    double raise_time;
    double lower_time;

    double max_car_wait;
    double max_ship_wait;

    double priority_prob_car;
    double priority_prob_ship;

    GroupStrategy group_strategy;
    size_t        max_group_size;

    int    max_served;
    double shift_end;

    unsigned long seed;
    double        display_delay;
} Config;

void config_defaults(Config *cfg);

bool config_parse_args(Config *cfg, int argc, char **argv);

void config_print(const Config *cfg);

const char *group_strategy_name(GroupStrategy s);

#endif /* CONFIG_H */
