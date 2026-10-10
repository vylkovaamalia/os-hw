#ifndef RNG_H
#define RNG_H

#include <stdint.h>
#include <stdbool.h>

// Воспроизводимый генератор псевдослучайных чисел. Алгоритм: xorshift64*
typedef struct {
    uint64_t state;
} Rng;

void rng_init(Rng *rng, uint64_t seed);

uint64_t rng_next_u64(Rng *rng);

// Равномерное целое в диапазоне [0, bound)
uint32_t rng_next_u32(Rng *rng, uint32_t bound);

// Равномерное вещественное в [0, 1)
double rng_next_double(Rng *rng);

// Интервал по экспоненциальному закону
double rng_exp_interval(Rng *rng, double lambda);

// Возвращает true с вероятностью p
bool rng_chance(Rng *rng, double p);

#endif /* RNG_H */
