/*
 * Control Variates Variance Reduction
 *
 * Control variates reduce MC variance by exploiting correlation with
 * a variable whose expectation is known analytically.
 *
 * Basic idea:
 *   Y = X - c·(Z - E[Z])
 *
 * Where:
 *   X = MC estimator (what we want)
 *   Z = Control variate (correlated with X, known E[Z])
 *   c = Optimal coefficient (usually estimated from simulation)
 *
 * Variance reduction:
 *   Var(Y) = Var(X)·(1 - ρ²)
 *
 * Where ρ is the correlation between X and Z.
 *
 * Common control variates for options:
 *
 * 1. Spot price as control:
 *    E[S(T)] = S(0)·e^(rT) is known
 *    Works well for delta-like payoffs
 *
 * 2. Geometric Asian as control for Arithmetic Asian:
 *    Geometric average has closed form
 *    Highly correlated with arithmetic average
 *
 * 3. European option as control for American:
 *    Black-Scholes gives E[European payoff]
 *    American payoff is correlated
 *
 * 4. Antithetic + Control (combined):
 *    Use both techniques together
 *
 * Implementation:
 *   1. Simulate paths, compute both X_i and Z_i for each path
 *   2. Estimate optimal c = Cov(X,Z) / Var(Z)
 *   3. Return mean(X) - c·(mean(Z) - E[Z])
 */

#ifndef MCO_INTERNAL_VARIANCE_REDUCTION_CONTROL_VARIATES_H
#define MCO_INTERNAL_VARIANCE_REDUCTION_CONTROL_VARIATES_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"
#include <stddef.h>

/*
 * Control variate type
 */
typedef enum {
    MCO_CV_NONE      = 0,  /* No control variate */
    MCO_CV_SPOT      = 1,  /* Use terminal spot as control */
    MCO_CV_GEOMETRIC = 2,  /* Use geometric average (for Asian) */
    MCO_CV_EUROPEAN  = 3,  /* Use European price (for American/Bermudan) */
    MCO_CV_DELTA     = 4   /* Use delta hedge as control */
} mco_cv_type;

/*
 * Control variate statistics
 */
typedef struct {
    double sum_x;       /* Sum of primary estimator */
    double sum_z;       /* Sum of control variate */
    double sum_xx;      /* Sum of X² */
    double sum_zz;      /* Sum of Z² */
    double sum_xz;      /* Sum of X·Z */
    double ez;          /* Known E[Z] */
    uint64_t n;         /* Sample count */
} mco_cv_stats;

/*
 * Initialize control variate statistics
 */
static inline void mco_cv_init(mco_cv_stats *stats, double ez)
{
    stats->sum_x  = 0.0;
    stats->sum_z  = 0.0;
    stats->sum_xx = 0.0;
    stats->sum_zz = 0.0;
    stats->sum_xz = 0.0;
    stats->ez     = ez;
    stats->n      = 0;
}

/*
 * Add a sample pair (x, z) to the statistics
 */
static inline void mco_cv_add(mco_cv_stats *stats, double x, double z)
{
    stats->sum_x  += x;
    stats->sum_z  += z;
    stats->sum_xx += x * x;
    stats->sum_zz += z * z;
    stats->sum_xz += x * z;
    stats->n++;
}

/*
 * Compute the control variate adjusted estimate
 *
 * Returns: mean(X) - c·(mean(Z) - E[Z])
 *
 * Where c = Cov(X,Z) / Var(Z) is estimated from samples
 */
static inline double mco_cv_estimate(const mco_cv_stats *stats)
{
    if (stats->n == 0) return 0.0;

    double n = (double)stats->n;
    double mean_x = stats->sum_x / n;
    double mean_z = stats->sum_z / n;

    /* Var(Z) = E[Z²] - E[Z]² */
    double var_z = (stats->sum_zz / n) - (mean_z * mean_z);

    if (var_z < 1e-12) {
        /* Control has no variance - just return mean_x */
        return mean_x;
    }

    /* Cov(X,Z) = E[XZ] - E[X]E[Z] */
    double cov_xz = (stats->sum_xz / n) - (mean_x * mean_z);

    /* Optimal coefficient */
    double c = cov_xz / var_z;

    /* Adjusted estimate */
    return mean_x - c * (mean_z - stats->ez);
}

/*
 * Get the estimated variance reduction factor (1 - ρ²)
 *
 * Values close to 0 indicate high variance reduction.
 * Values close to 1 indicate little benefit.
 */
static inline double mco_cv_variance_reduction(const mco_cv_stats *stats)
{
    if (stats->n < 2) return 1.0;

    double n = (double)stats->n;
    double mean_x = stats->sum_x / n;
    double mean_z = stats->sum_z / n;

    double var_x = (stats->sum_xx / n) - (mean_x * mean_x);
    double var_z = (stats->sum_zz / n) - (mean_z * mean_z);

    if (var_x < 1e-12 || var_z < 1e-12) return 1.0;

    double cov_xz = (stats->sum_xz / n) - (mean_x * mean_z);
    double rho_sq = (cov_xz * cov_xz) / (var_x * var_z);

    return 1.0 - rho_sq;
}

/*============================================================================
 * Convenience Functions
 *============================================================================*/

/*
 * Price European option with spot as control variate
 *
 * Uses E[S(T)] = S(0)·e^(rT) as control
 */
double mco_european_cv_spot(mco_ctx *ctx,
                            double spot,
                            double strike,
                            double rate,
                            double volatility,
                            double time_to_maturity,
                            mco_option_type type);

/*
 * Price arithmetic Asian with geometric Asian as control
 *
 * Geometric Asian has closed-form, highly correlated with arithmetic
 */
double mco_asian_cv_geometric(mco_ctx *ctx,
                              double spot,
                              double strike,
                              double rate,
                              double volatility,
                              double time_to_maturity,
                              size_t num_obs,
                              mco_option_type type);

#endif /* MCO_INTERNAL_VARIANCE_REDUCTION_CONTROL_VARIATES_H */
