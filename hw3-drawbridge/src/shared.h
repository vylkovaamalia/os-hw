#ifndef SHARED_H
#define SHARED_H

#include <signal.h>
#include <semaphore.h>
#include <sys/types.h>
#include "config.h"
#include "bridge.h"
#include "states.h"

#define SHM_NAME       "/bridge_shm_v41"
#define SEM_MUTEX_NAME "/bridge_sem_mutex_v41"
#define SEM_LOG_NAME   "/bridge_sem_log_v41"
#define SEM_EVENT_NAME "/bridge_sem_event_v41"
#define SEM_MECH_NAME  "/bridge_sem_mech_v41"

#define MAX_CARS   256
#define MAX_SHIPS  64
#define MAX_GROUP  64

typedef struct {
    int      id;
    Priority priority;
    CarState state;
    double   arrival_time;
    pid_t    pid;
    int      used;
} CarDesc;

typedef struct {
    int           id;
    ShipDirection direction;
    Priority      priority;
    ShipState     state;
    double        arrival_time;
    pid_t         pid;
    int           group_id;
    int           used;
} ShipDesc;

typedef struct {
    size_t head;
    size_t tail;
    size_t size;
    size_t capacity;
} RingHeader;

typedef enum {
    MECH_IDLE = 0,
    MECH_CLOSE_BARRIER,
    MECH_RAISE,
    MECH_LOWER,
    MECH_OPEN_BARRIER,
    MECH_STOP
} MechCmd;

typedef struct {
    Config    cfg;
    Bridge    bridge;

    CarDesc   car_slots[MAX_CARS];
    int       cars_normal_idx[MAX_CARS];
    int       cars_priority_idx[MAX_CARS];
    RingHeader cars_normal;
    RingHeader cars_priority;

    ShipDesc  ship_slots[MAX_SHIPS];
    int       ships_up_normal_idx[MAX_SHIPS];
    int       ships_up_priority_idx[MAX_SHIPS];
    int       ships_down_normal_idx[MAX_SHIPS];
    int       ships_down_priority_idx[MAX_SHIPS];
    RingHeader ships_up_normal;
    RingHeader ships_up_priority;
    RingHeader ships_down_normal;
    RingHeader ships_down_priority;

    int       ship_group[MAX_GROUP];
    int       ship_group_size;
    int       ships_on_bridge;

    unsigned  served_total;
    unsigned  cars_served;
    unsigned  ships_served;
    unsigned  cars_rejected;
    unsigned  ships_rejected;

    int       next_car_id;
    int       next_ship_id;

    MechCmd   mechanism_cmd;

    volatile sig_atomic_t stop;
} SharedState;

#endif