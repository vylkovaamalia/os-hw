#ifndef OPERATOR_H
#define OPERATOR_H

#include "states.h"
#include "stream.h"
#include "bridge.h"
#include "transport.h"

typedef struct {
    GroupStrategy strategy;

    int           current_group_id;
    int           current_group_size;
} Operator;

void operator_init(Operator *op, GroupStrategy strat);

bool operator_review(Operator *op,
                     Bridge *bridge,
                     const Stream *cars,
                     const Stream *ships_up,
                     const Stream *ships_down,
                     Time now);

size_t operator_form_ship_group(Operator *op,
                                Stream *ships_up,
                                Stream *ships_down,
                                Ship **out_ships,
                                size_t max_size);

void operator_notify_timeout(Operator *op, Id entity_id);

#endif /* OPERATOR_H */
