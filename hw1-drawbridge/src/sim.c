#include "config.h"
#include "sim.h"
#include <stdlib.h>
#include <stdio.h>

volatile sig_atomic_t g_stop_requested = 0;

bool sim_init(Simulation *sim, const Config *cfg) {
    if (!sim || !cfg) return false;

    sim->cfg = *cfg;
    logger_init(&sim->logger, "sim.log", cfg->display_delay);
    rng_init(&sim->rng, cfg->seed);

    BridgeTiming t = {
        .barrier_close_time = cfg->barrier_close_time,
        .barrier_open_time  = cfg->barrier_open_time,
        .raise_time         = cfg->raise_time,
        .lower_time         = cfg->lower_time,
    };
    bridge_init(&sim->bridge, &t);
    operator_init(&sim->op, cfg->group_strategy);

    if (!stream_init(&sim->cars, cfg->car_queue_cap)) {
        logger_close(&sim->logger);
        return false;
    }
    if (!stream_init(&sim->ships_up, cfg->ship_queue_cap)) {
        stream_free(&sim->cars);
        logger_close(&sim->logger);
        return false;
    }
    if (!stream_init(&sim->ships_down, cfg->ship_queue_cap)) {
        stream_free(&sim->cars);
        stream_free(&sim->ships_up);
        logger_close(&sim->logger);
        return false;
    }
    if (!event_queue_init(&sim->events, 4096)) {
        stream_free(&sim->cars);
        stream_free(&sim->ships_up);
        stream_free(&sim->ships_down);
        logger_close(&sim->logger);
        return false;
    }

    sim->now          = 0.0;
    sim->next_car_id  = 1;
    sim->next_ship_id = 1;

    sim->served_total   = 0;
    sim->cars_served    = 0;
    sim->ships_served   = 0;
    sim->cars_rejected  = 0;
    sim->ships_rejected = 0;

    return true;
}

static void drain_cars(Stream *s) {
    void *item;
    while ((item = stream_pop(s)) != NULL) car_destroy((Car *) item);
}

static void drain_ships(Stream *s) {
    void *item;
    while ((item = stream_pop(s)) != NULL) ship_destroy((Ship *) item);
}

void sim_free(Simulation *sim) {
    if (!sim) return;
    drain_cars(&sim->cars);
    drain_ships(&sim->ships_up);
    drain_ships(&sim->ships_down);
    event_queue_free(&sim->events);
    stream_free(&sim->cars);
    stream_free(&sim->ships_up);
    stream_free(&sim->ships_down);
    logger_close(&sim->logger);
}


static void schedule_car_arrival(Simulation *sim) {
    double dt = rng_exp_interval(&sim->rng, sim->cfg.car_lambda);
    bool prio = rng_chance(&sim->rng, sim->cfg.priority_prob_car);

    Event ev = {0};
    ev.time     = sim->now + dt;
    ev.type     = prio ? EV_PRIORITY_CAR_ARRIVAL : EV_CAR_ARRIVAL;
    ev.priority = prio ? EMERGENCY : NORMAL;
    event_queue_push(&sim->events, ev);
}

static void schedule_ship_arrival(Simulation *sim, ShipDirection dir) {
    double lambda = (dir == SHIP_UP) ? sim->cfg.ship_lambda_up
                                     : sim->cfg.ship_lambda_down;
    double dt   = rng_exp_interval(&sim->rng, lambda);
    bool   prio = rng_chance(&sim->rng, sim->cfg.priority_prob_ship);

    Event ev = {0};
    ev.time      = sim->now + dt;
    ev.type      = prio ? EV_PRIORITY_SHIP_ARRIVAL : EV_SHIP_ARRIVAL;
    ev.direction = dir;
    ev.priority  = prio ? EMERGENCY : NORMAL;
    event_queue_push(&sim->events, ev);
}

static void schedule_bridge_completion(Simulation *sim) {
    Event ev = {0};
    ev.time = sim->bridge.state_end_time;

    switch (sim->bridge.state) {
    case CLOSE_BARRIER: ev.type = EV_BARRIER_CLOSE_DONE; break;
    case RAISE:         ev.type = EV_RAISE_DONE;         break;
    case LOWER:         ev.type = EV_LOWER_DONE;         break;
    case OPEN_BARRIER:  ev.type = EV_BARRIER_OPEN_DONE;  break;
    default: return;   /* нечего планировать */
    }
    event_queue_push(&sim->events, ev);
}


static void dispatch_cars(Simulation *sim) {
    if (sim->bridge.state != ROAD_OPEN) return;

    while (!stream_is_empty(&sim->cars)) {
        Car *c = (Car *) stream_pop(&sim->cars);

        Event ev = {0};
        ev.time      = sim->now;
        ev.type      = EV_CAR_ENTER;
        ev.entity_id = c->id;
        ev.payload   = c;
        event_queue_push(&sim->events, ev);
    }
}

static void dispatch_ships(Simulation *sim) {
    Ship *group[SIM_MAX_GROUP];
    size_t cap = sim->cfg.max_group_size;
    if (cap > SIM_MAX_GROUP) cap = SIM_MAX_GROUP;

    size_t n = operator_form_ship_group(&sim->op,
                                        &sim->ships_up,
                                        &sim->ships_down,
                                        group, cap);

    sim->bridge.ships_passing = (int) n;

    for (size_t i = 0; i < n; ++i) {
        Event ev = {0};
        ev.time      = sim->now;
        ev.type      = EV_SHIP_ENTER;
        ev.entity_id = group[i]->id;
        ev.direction = group[i]->direction;
        ev.payload   = group[i];
        event_queue_push(&sim->events, ev);
    }
}


static void handle_car_arrival(Simulation *sim, const Event *ev) {
    if (stream_is_full(&sim->cars)) {
        sim->cars_rejected++;
        schedule_car_arrival(sim);
        return;
    }

    Car *c = car_create(sim->next_car_id++, ev->priority,
                        ev->time, sim->cfg.max_car_wait);
    if (!c) return;

    stream_push(&sim->cars, c, ev->priority == EMERGENCY);
    schedule_car_arrival(sim);
}

static void handle_ship_arrival(Simulation *sim, const Event *ev) {
    Stream *target = (ev->direction == SHIP_UP) ? &sim->ships_up
                                                : &sim->ships_down;
    if (stream_is_full(target)) {
        sim->ships_rejected++;
        schedule_ship_arrival(sim, ev->direction);
        return;
    }

    Ship *s = ship_create(sim->next_ship_id++, ev->direction,
                          ev->priority, ev->time, sim->cfg.max_ship_wait);
    if (!s) return;

    stream_push(target, s, ev->priority == EMERGENCY);
    schedule_ship_arrival(sim, ev->direction);
}

static void handle_car_enter(Simulation *sim, const Event *ev) {
    Car *c = (Car *) ev->payload;
    if (!c) return;

    c->state             = CAR_ON_BRIDGE;
    c->bridge_enter_time = sim->now;
    sim->bridge.cars_on_bridge++;

    Event out = {0};
    out.time      = sim->now + sim->cfg.car_cross_time;
    out.type      = EV_CAR_EXIT;
    out.entity_id = c->id;
    out.payload   = c;
    event_queue_push(&sim->events, out);
}

static void handle_car_exit(Simulation *sim, const Event *ev) {
    Car *c = (Car *) ev->payload;
    if (!c) return;

    c->state       = CAR_DONE;
    c->finish_time = sim->now;

    sim->bridge.cars_on_bridge--;
    sim->cars_served++;
    sim->served_total++;

    car_destroy(c);
}

static void handle_ship_enter(Simulation *sim, const Event *ev) {
    Ship *s = (Ship *) ev->payload;
    if (!s) return;

    s->state           = SHIP_PASSING;
    s->pass_start_time = sim->now;

    Event out = {0};
    out.time      = sim->now + sim->cfg.ship_pass_time;
    out.type      = EV_SHIP_EXIT;
    out.entity_id = s->id;
    out.direction = s->direction;
    out.payload   = s;
    event_queue_push(&sim->events, out);
}

static void handle_ship_exit(Simulation *sim, const Event *ev) {
    Ship *s = (Ship *) ev->payload;
    if (!s) return;

    s->state       = SHIP_DONE;
    s->finish_time = sim->now;

    sim->bridge.ships_passing--;
    sim->ships_served++;
    sim->served_total++;

    ship_destroy(s);
}

static void handle_barrier_close_done(Simulation *sim, const Event *ev) {
    (void) ev;
    bridge_finish_operation(&sim->bridge, sim->now);
}

static void handle_barrier_open_done(Simulation *sim, const Event *ev) {
    (void) ev;
    bridge_finish_operation(&sim->bridge, sim->now);
}

static void handle_lower_done(Simulation *sim, const Event *ev) {
    (void) ev;
    bridge_finish_operation(&sim->bridge, sim->now);
}

static void handle_raise_done(Simulation *sim, const Event *ev) {
    (void) ev;
    bridge_finish_operation(&sim->bridge, sim->now);
    dispatch_ships(sim);
}

static void handle_shift_end(Simulation *sim, const Event *ev) {
    (void) sim; (void) ev;
    g_stop_requested = 1;
}

static void handle_event(Simulation *sim, const Event *ev) {
    switch (ev->type) {
    case EV_CAR_ARRIVAL:
    case EV_PRIORITY_CAR_ARRIVAL:
        handle_car_arrival(sim, ev);
        break;

    case EV_SHIP_ARRIVAL:
    case EV_PRIORITY_SHIP_ARRIVAL:
        handle_ship_arrival(sim, ev);
        break;

    case EV_BARRIER_CLOSE_DONE: handle_barrier_close_done(sim, ev); break;
    case EV_RAISE_DONE:         handle_raise_done(sim, ev);         break;
    case EV_SHIP_ENTER:         handle_ship_enter(sim, ev);         break;
    case EV_SHIP_EXIT:          handle_ship_exit(sim, ev);          break;
    case EV_LOWER_DONE:         handle_lower_done(sim, ev);         break;
    case EV_BARRIER_OPEN_DONE:  handle_barrier_open_done(sim, ev);  break;

    case EV_CAR_ENTER:          handle_car_enter(sim, ev);          break;
    case EV_CAR_EXIT:           handle_car_exit(sim, ev);           break;

    case EV_SHIFT_END:
        handle_shift_end(sim, ev);
        break;

    default:
        break;
    }
}


static bool should_finish(const Simulation *sim) {
    if (sim->cfg.max_served > 0 &&
        (int) sim->served_total >= sim->cfg.max_served) {
        return true;
    }
    return false;
}


int sim_run(Simulation *sim) {
    if (!sim) return 1;

    schedule_car_arrival(sim);
    schedule_ship_arrival(sim, SHIP_UP);
    schedule_ship_arrival(sim, SHIP_DOWN);

    if (sim->cfg.shift_end > 0.0) {
        Event ev = {0};
        ev.time = sim->cfg.shift_end;
        ev.type = EV_SHIFT_END;
        event_queue_push(&sim->events, ev);
    }

    while (!g_stop_requested) {
        Event ev;
        if (!event_queue_pop_min(&sim->events, &ev)) break;

        sim->now = ev.time;
        handle_event(sim, &ev);
        logger_event(&sim->logger, sim->now, &ev);

        if (!bridge_check_invariants(&sim->bridge)) {
            fprintf(stderr, "[t=%.2f] INVARIANT VIOLATION, state=%s\n",
                    sim->now, bridge_state_name(sim->bridge.state));
            return 1;
        }


        bool action = operator_review(&sim->op, &sim->bridge,
                                      &sim->cars,
                                      &sim->ships_up, &sim->ships_down,
                                      sim->now);
        if (action) {
            schedule_bridge_completion(sim);
        }

        dispatch_cars(sim);

        if (should_finish(sim)) break;
    }

    logger_summary(&sim->logger,
                    sim->now,
                    sim->cars_served,
                    sim->ships_served,
                    sim->cars_rejected,
                    sim->ships_rejected,
                    stream_size(&sim->cars),
                    stream_size(&sim->ships_up),
                    stream_size(&sim->ships_down));

    return 0;
}
