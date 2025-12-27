/*
 * Xoshiro256** pseudo-random number generator
 *
 * Reference: https://prng.di.unimi.it/
 * Authors: David Blackman and Sebastiano Vigna (2018)
 *
 * Properties:
 *   - Period: 2^256 - 1
 *   - State: 256 bits (4 x uint64_t)
 *   - Speed: ~0.8ns per 64-bit output on modern x86
 *   - Quality: Passes BigCrush, PractRand
 *   - Jumpable: Can advance state by 2^128 steps (for parallel streams)
 *
 * Why not Mersenne Twister?
 *   - MT19937 has 2.5KB state vs 32 bytes here
 *   - MT is slower and has known statistical weaknesses
 *   - Xoshiro is trivially parallelizable via jump()
 */

#ifndef MCO_INTERNAL_RNG_H
#define MCO_INTERNAL_RNG_H

#include <stdint.h>

typedef struct {
    uint64_t s[4];
} mco_rng;

/* Initialize RNG from a 64-bit seed using SplitMix64 */
void mco_rng_seed(mco_rng *rng, uint64_t seed);

/* Generate uniform random uint64_t in [0, 2^64) */
uint64_t mco_rng_next(mco_rng *rng);

/* Generate uniform random double in [0, 1) */
double mco_rng_uniform(mco_rng *rng);

/* Generate standard normal via Box-Muller */
double mco_rng_normal(mco_rng *rng);

/*
 * Jump the RNG state forward by 2^128 steps.
 * Use this to create independent streams for parallel threads.
 *
 * Example for 4 threads:
 *   mco_rng base; mco_rng_seed(&base, user_seed);
 *   mco_rng t0 = base;
 *   mco_rng t1 = base; mco_rng_jump(&t1);
 *   mco_rng t2 = t1;   mco_rng_jump(&t2);
 *   mco_rng t3 = t2;   mco_rng_jump(&t3);
 */
void mco_rng_jump(mco_rng *rng);

#endif /* MCO_INTERNAL_RNG_H */
