/*
 * Xoshiro256** pseudo-random number generator
 *
 * Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
 * Public domain - see https://prng.di.unimi.it/
 *
 * This is xoshiro256** 1.0, one of our all-purpose, rock-solid generators.
 * It has excellent (sub-ns) speed, a state (256 bits) that is large enough
 * for any parallel application, and it passes all tests we are aware of.
 *
 * Modifications for mcoptions:
 *   - Wrapped in mco_rng struct
 *   - Added Box-Muller normal generation
 *   - Added SplitMix64 seeding
 */

#include "internal/rng.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*============================================================================
 * SplitMix64 - Used only for seeding
 *============================================================================*/

/*
 * SplitMix64 is a fast, high-quality generator suitable for seeding.
 * We use it to expand a single 64-bit seed into 256 bits of state.
 */
static inline uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

/*============================================================================
 * Xoshiro256** Core
 *============================================================================*/

static inline uint64_t rotl(const uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

/*
 * Generate next 64-bit random value.
 *
 * This is the core xoshiro256** algorithm.
 * ~0.8ns per call on modern x86-64.
 */
uint64_t mco_rng_next(mco_rng *rng)
{
    const uint64_t result = rotl(rng->s[1] * 5, 7) * 9;

    const uint64_t t = rng->s[1] << 17;

    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];

    rng->s[2] ^= t;

    rng->s[3] = rotl(rng->s[3], 45);

    return result;
}

/*
 * Seed the RNG from a single 64-bit value.
 *
 * Uses SplitMix64 to expand the seed into 256 bits.
 * Ensures state is never all-zeros (which would be a fixed point).
 */
void mco_rng_seed(mco_rng *rng, uint64_t seed)
{
    uint64_t sm_state = seed;

    rng->s[0] = splitmix64(&sm_state);
    rng->s[1] = splitmix64(&sm_state);
    rng->s[2] = splitmix64(&sm_state);
    rng->s[3] = splitmix64(&sm_state);

    /* Ensure non-zero state (astronomically unlikely with SplitMix64) */
    if (rng->s[0] == 0 && rng->s[1] == 0 && rng->s[2] == 0 && rng->s[3] == 0) {
        rng->s[0] = 1;
    }
}

/*============================================================================
 * Uniform Distribution
 *============================================================================*/

/*
 * Generate uniform double in [0, 1).
 *
 * Uses the upper 53 bits of a 64-bit value to fill the mantissa
 * of a double-precision float. This gives the maximum precision
 * possible for a double in [0, 1).
 */
double mco_rng_uniform(mco_rng *rng)
{
    uint64_t x = mco_rng_next(rng);
    return (double)(x >> 11) * 0x1.0p-53;
}

/*============================================================================
 * Normal Distribution (Box-Muller)
 *============================================================================*/

/*
 * Generate standard normal (mean=0, stddev=1) using Box-Muller transform.
 *
 * Box-Muller generates pairs of independent normals from pairs of uniforms:
 *
 *   Z0 = sqrt(-2 * ln(U1)) * cos(2π * U2)
 *   Z1 = sqrt(-2 * ln(U1)) * sin(2π * U2)
 *
 * We compute both but only return one. The discarded value is the cost
 * of simplicity - we could cache it, but that complicates the state.
 *
 * Note: We use U1 in (0, 1] to avoid ln(0). Adding a small epsilon
 * or using 1-U achieves this.
 */
double mco_rng_normal(mco_rng *rng)
{
    double u1 = mco_rng_uniform(rng);
    double u2 = mco_rng_uniform(rng);

    /* Avoid log(0) - transform [0,1) to (0,1] */
    u1 = 1.0 - u1;

    double r = sqrt(-2.0 * log(u1));
    double theta = 2.0 * M_PI * u2;

    return r * cos(theta);
}

/*============================================================================
 * Jump Function (for Parallel Streams)
 *============================================================================*/

/*
 * Jump the RNG forward by 2^128 steps.
 *
 * This is equivalent to calling mco_rng_next() 2^128 times, but computed
 * in O(256) operations using the jump polynomial.
 *
 * Use case: Create independent streams for parallel threads.
 *
 *   mco_rng base; mco_rng_seed(&base, user_seed);
 *   mco_rng t0 = base;                           // Thread 0
 *   mco_rng t1 = base; mco_rng_jump(&t1);        // Thread 1: 2^128 ahead
 *   mco_rng t2 = t1;   mco_rng_jump(&t2);        // Thread 2: 2^128 ahead of t1
 *   ...
 *
 * With 2^128 steps between streams, there's no risk of overlap even with
 * billions of threads running trillions of simulations each.
 */
void mco_rng_jump(mco_rng *rng)
{
    static const uint64_t JUMP[] = {
        0x180ec6d33cfd0aba,
        0xd5a61266f0c9392c,
        0xa9582618e03fc9aa,
        0x39abdc4529b1661c
    };

    uint64_t s0 = 0;
    uint64_t s1 = 0;
    uint64_t s2 = 0;
    uint64_t s3 = 0;

    for (int i = 0; i < 4; i++) {
        for (int b = 0; b < 64; b++) {
            if (JUMP[i] & ((uint64_t)1 << b)) {
                s0 ^= rng->s[0];
                s1 ^= rng->s[1];
                s2 ^= rng->s[2];
                s3 ^= rng->s[3];
            }
            mco_rng_next(rng);
        }
    }

    rng->s[0] = s0;
    rng->s[1] = s1;
    rng->s[2] = s2;
    rng->s[3] = s3;
}
