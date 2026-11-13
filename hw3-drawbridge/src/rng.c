#include "rng.h"
#include <math.h>

void rng_init(Rng *rng, uint64_t seed) {
    rng->state = seed ? seed : 0x9E3779B97F4A7C15ULL;
}

uint64_t rng_next_u64(Rng *rng) {
    uint64_t x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

uint32_t rng_next_u32(Rng *rng, uint32_t bound) {
    if (bound == 0) return 0;
    uint32_t r = (uint32_t)(rng_next_u64(rng) >> 32);
    return r % bound;
}

double rng_next_double(Rng *rng) {
    uint64_t r = rng_next_u64(rng) >> 11;
    return (double)r / 9007199254740992.0;
}

double rng_exp_interval(Rng *rng, double lambda) {
    if (lambda <= 0.0) return 0.0;
    double u;
    do {
        u = rng_next_double(rng);
    } while (u <= 0.0);
    return -log(u) / lambda;
}

bool rng_chance(Rng *rng, double p) {
    if (p <= 0.0) return false;
    if (p >= 1.0) return true;
    return rng_next_double(rng) < p;
}