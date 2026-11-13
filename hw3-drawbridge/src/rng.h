#ifndef RNG_H
#define RNG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t state;
} Rng;

void     rng_init(Rng *rng, uint64_t seed);
uint64_t rng_next_u64(Rng *rng);
uint32_t rng_next_u32(Rng *rng, uint32_t bound);
double   rng_next_double(Rng *rng);
double   rng_exp_interval(Rng *rng, double lambda);
bool     rng_chance(Rng *rng, double p);

#endif