/*
 * Thread pool for parallel Monte Carlo simulation
 *
 * Design:
 *   - Each thread gets its own RNG state (via jump() for reproducibility)
 *   - Work is divided into chunks, one per thread
 *   - Results are accumulated without locks (each thread writes to its own slot)
 *
 * Threading model:
 *   - Single-threaded (num_threads=1): No pthread overhead, direct execution
 *   - Multi-threaded: Spawns N threads, each processes 1/N of simulations
 *
 * Reproducibility:
 *   - Same seed + same thread count = same results
 *   - Thread i uses RNG state = jump(base_rng, i)
 */

#ifndef MCO_INTERNAL_METHODS_THREAD_POOL_H
#define MCO_INTERNAL_METHODS_THREAD_POOL_H

#include "internal/context.h"
#include "internal/rng.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Thread-local work context passed to each worker
 */
typedef struct {
    mco_rng rng;              /* Thread-local RNG state */
    uint64_t start_sim;       /* First simulation index (inclusive) */
    uint64_t end_sim;         /* Last simulation index (exclusive) */
    double partial_sum;       /* Accumulated payoff sum */
    double partial_sum_sq;    /* Accumulated payoff^2 (for variance) */

    /* Option parameters (copied for cache locality) */
    double spot;
    double strike;
    double rate;
    double volatility;
    double time_to_maturity;
    int option_type;          /* MCO_CALL or MCO_PUT */
    int antithetic;           /* Use antithetic variates */
} mco_thread_work;

/*
 * Initialize thread work contexts for parallel execution.
 *
 * Divides simulations evenly among threads and initializes
 * each thread's RNG via jump() for reproducibility.
 *
 * Parameters:
 *   work        - Array of mco_thread_work, size = num_threads
 *   num_threads - Number of threads
 *   base_rng    - Master RNG (will be copied and jumped)
 *   total_sims  - Total number of simulations
 *
 * After this call, each work[i] has:
 *   - Unique RNG state (jumped i times from base)
 *   - start_sim and end_sim defining its chunk
 *   - partial_sum = 0
 */
void mco_thread_work_init(mco_thread_work *work,
                          uint32_t num_threads,
                          const mco_rng *base_rng,
                          uint64_t total_sims);

/*
 * Execute European option pricing in parallel.
 *
 * Parameters:
 *   ctx   - Context with thread count and simulation parameters
 *   spot, strike, rate, volatility, time_to_maturity - Option params
 *   type  - MCO_CALL or MCO_PUT
 *
 * Returns:
 *   Option price (discounted expected payoff)
 */
double mco_parallel_european(mco_ctx *ctx,
                             double spot,
                             double strike,
                             double rate,
                             double volatility,
                             double time_to_maturity,
                             int type);

#endif /* MCO_INTERNAL_METHODS_THREAD_POOL_H */
