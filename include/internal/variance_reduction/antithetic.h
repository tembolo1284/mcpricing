/*
 * Antithetic Variates
 *
 * The simplest variance reduction technique for Monte Carlo simulation.
 *
 * Idea:
 *   Instead of generating N independent paths, generate N/2 pairs
 *   where each pair uses (Z, -Z) for the same random draw Z.
 *
 * Why it works:
 *   If f(Z) and f(-Z) are negatively correlated, their average
 *   has lower variance than two independent samples:
 *
 *   Var[(f(Z) + f(-Z))/2] = (Var[f(Z)] + Cov[f(Z), f(-Z)]) / 2
 *
 *   When Cov < 0, variance is reduced.
 *
 * For GBM:
 *   S⁺(T) = S₀ · exp((r - σ²/2)T + σ√T·Z)
 *   S⁻(T) = S₀ · exp((r - σ²/2)T - σ√T·Z)
 *
 *   The payoffs max(S⁺-K, 0) and max(S⁻-K, 0) are negatively correlated
 *   because when S⁺ is high, S⁻ is low and vice versa.
 *
 * Effectiveness:
 *   - European options: ~50% variance reduction typical
 *   - Path-dependent: varies, generally beneficial
 *   - Cost: nearly zero (just negate Z)
 *   - Always worth using
 *
 * Implementation notes:
 *   - Use N/2 random draws to generate N paths
 *   - Each draw produces two payoffs: f(+Z) and f(-Z)
 *   - Average both payoffs into the running sum
 *   - Final average is over N paths, not N/2
 */

#ifndef MCO_INTERNAL_VARIANCE_REDUCTION_ANTITHETIC_H
#define MCO_INTERNAL_VARIANCE_REDUCTION_ANTITHETIC_H

#include "internal/models/gbm.h"
#include "internal/instruments/payoff.h"
#include "internal/rng.h"
#include <stdint.h>

/*
 * Simulate European option payoffs with antithetic variates.
 *
 * Generates num_pairs pairs of paths using (Z, -Z).
 * Returns sum of all 2*num_pairs payoffs (NOT averaged).
 *
 * Parameters:
 *   model     - Initialized GBM model
 *   rng       - Thread-local RNG
 *   strike    - Strike price
 *   type      - MCO_CALL or MCO_PUT
 *   num_pairs - Number of (Z, -Z) pairs to simulate
 *
 * Returns:
 *   Sum of payoffs (caller divides by 2*num_pairs and discounts)
 */
static inline double mco_antithetic_european_sum(const mco_gbm *model,
                                                  mco_rng *rng,
                                                  double strike,
                                                  mco_option_type type,
                                                  uint64_t num_pairs)
{
    double sum = 0.0;

    for (uint64_t i = 0; i < num_pairs; ++i) {
        double z = mco_rng_normal(rng);

        double spot_plus  = mco_gbm_terminal(model, z);
        double spot_minus = mco_gbm_terminal(model, -z);

        sum += mco_payoff(spot_plus, strike, type);
        sum += mco_payoff(spot_minus, strike, type);
    }

    return sum;
}

/*
 * Simulate European option with antithetic variates.
 *
 * Convenience function that does the full calculation:
 *   price = discount * mean(payoffs)
 *
 * Parameters:
 *   model       - Initialized GBM model (includes discount factor)
 *   rng         - Thread-local RNG
 *   strike      - Strike price
 *   type        - MCO_CALL or MCO_PUT
 *   num_sims    - Total number of paths (will use num_sims/2 pairs)
 *
 * Returns:
 *   Option price
 */
static inline double mco_antithetic_european(const mco_gbm *model,
                                              mco_rng *rng,
                                              double strike,
                                              mco_option_type type,
                                              uint64_t num_sims)
{
    uint64_t num_pairs = num_sims / 2;
    if (num_pairs == 0) num_pairs = 1;

    double sum = mco_antithetic_european_sum(model, rng, strike, type, num_pairs);
    double mean = sum / (double)(2 * num_pairs);

    return model->discount * mean;
}

/*
 * Path-dependent antithetic simulation
 *
 * For Asian, Barrier, Lookback options, we need full paths.
 * Generate two paths simultaneously using (Z_i, -Z_i) at each step.
 *
 * Parameters:
 *   model      - Initialized GBM path model
 *   rng        - Thread-local RNG
 *   path_plus  - Output array for +Z path, size (num_steps + 1)
 *   path_minus - Output array for -Z path, size (num_steps + 1)
 */
static inline void mco_antithetic_path(const mco_gbm_path *model,
                                        mco_rng *rng,
                                        double *path_plus,
                                        double *path_minus)
{
    path_plus[0]  = model->spot;
    path_minus[0] = model->spot;

    for (size_t i = 0; i < model->num_steps; ++i) {
        double z = mco_rng_normal(rng);

        path_plus[i + 1]  = mco_gbm_step(model, path_plus[i], z);
        path_minus[i + 1] = mco_gbm_step(model, path_minus[i], -z);
    }
}

#endif /* MCO_INTERNAL_VARIANCE_REDUCTION_ANTITHETIC_H */
