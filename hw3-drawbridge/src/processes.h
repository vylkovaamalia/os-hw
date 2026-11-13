#ifndef PROCESSES_H
#define PROCESSES_H

#include "shared.h"

void operator_process_main (SharedState *shm);
void mechanism_process_main(SharedState *shm);
void car_generator_main    (SharedState *shm);
void car_process_main      (SharedState *shm, int slot_idx);
void ship_generator_main   (SharedState *shm, ShipDirection dir);
void ship_process_main     (SharedState *shm, int slot_idx);

#endif