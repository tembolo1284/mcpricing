/*
 * Monte Carlo simulation methods
 *
 * Core path simulation for Geometric Brownian Motion (GBM).
 * Thread-safe: each call uses its own RNG state.
 */

#ifndef MCO_INTERNAL_METHODS_MONTE_CARLO_H
#define MCO_INTERNAL_METHODS_MONTE_CARLO_H

#include "internal/rng.h"
#include <stddef.h>

/*
 * Simulate a single GBM path and return the final spot price.
 *
 * Uses the exact solution:
 *   S(t) = S(0) * exp((r - 0.5*σ²)*t + σ*√t*Z)
 *
 * For European options, we only need the terminal value,
 * so we can skip intermediate steps entirely.
 *
 * Parameters:
 *   rng        - Thread-local RNG state
 *   spot       - Initial spot price
 *   rate       - Risk-free rate
 *   volatility - Annualized volatility
 *   time       - Time to maturity
 *
 * Returns:
 *   Terminal spot price S(T)
 */
double mco_simulate_terminal(mco_rng *rng,
                             double spot,
                             double rate,
                             double volatility,
                             double time);

/*
 * Simulate a single GBM path and return the final spot price.
 * Also returns the antithetic path result via out parameter.
 *
 * Parameters:
 *   rng            - Thread-local RNG state
 *   spot           - Initial spot price
 *   rate           - Risk-free rate
 *   volatility     - Annualized volatility
 *   time           - Time to maturity
 *   antithetic_out - Output: terminal price using -Z
 *
 * Returns:
 *   Terminal spot price S(T) using +Z
 */
double mco_simulate_terminal_antithetic(mco_rng *rng,
                                        double spot,
                                        double rate,
                                        double volatility,
                                        double time,
                                        double *antithetic_out);

/*
 * Simulate a full GBM path with discrete time steps.
 * Required for path-dependent options (Asian, Barrier, Lookback).
 *
 * Parameters:
 *   rng        - Thread-local RNG state
 *   path       - Output array of size (num_steps + 1)
 *   spot       - Initial spot price
 *   rate       - Risk-free rate
 *   volatility - Annualized volatility
 *   time       - Time to maturity
 *   num_steps  - Number of time steps
 *
 * The path array will contain:
 *   path[0] = spot (initial)
 *   path[1] = S(dt)
 *   ...
 *   path[num_steps] = S(T)
 */
void mco_simulate_path(mco_rng *rng,
                       double *path,
                       double spot,
                       double rate,
                       double volatility,
                       double time,
                       size_t num_steps);

/*
 * Discount factor: exp(-r * t)
 */
static inline double mco_discount(double rate, double time)
{
    extern double exp(double);
    return exp(-rate * time);
}

#endif /* MCO_INTERNAL_METHODS_MONTE_CARLO_H */
