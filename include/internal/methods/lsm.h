/*
 * Least Squares Monte Carlo (LSM) for American Options
 *
 * The Longstaff-Schwartz (2001) algorithm for pricing American options
 * via Monte Carlo simulation with regression-based early exercise decisions.
 *
 * Algorithm:
 *   1. Simulate N paths forward to maturity
 *   2. At maturity, compute terminal payoffs
 *   3. Work backwards through exercise dates:
 *      a. For in-the-money paths, regress continuation value on spot price
 *      b. Compare immediate exercise vs estimated continuation
 *      c. Exercise early if immediate > continuation
 *   4. Discount the optimal exercise payoffs to time 0
 *
 * Regression:
 *   Uses weighted Laguerre polynomials as basis functions:
 *     L0(x) = 1
 *     L1(x) = 1 - x
 *     L2(x) = 1 - 2x + x²/2
 *   
 *   Regression: E[V|S] ≈ β0·L0(S/K) + β1·L1(S/K) + β2·L2(S/K)
 *
 * Reference:
 *   Longstaff, F.A. and Schwartz, E.S. (2001)
 *   "Valuing American Options by Simulation: A Simple Least-Squares Approach"
 *   Review of Financial Studies, 14(1), 113-147
 */

#ifndef MCO_INTERNAL_METHODS_LSM_H
#define MCO_INTERNAL_METHODS_LSM_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"
#include <stddef.h>

/*
 * Number of basis functions for regression.
 * Using 3 Laguerre polynomials (constant, linear, quadratic).
 */
#define MCO_LSM_NUM_BASIS 3

/*
 * Price an American option using Least Squares Monte Carlo.
 *
 * Parameters:
 *   ctx              - Simulation context
 *   spot             - Current spot price
 *   strike           - Strike price
 *   rate             - Risk-free rate
 *   volatility       - Volatility
 *   time_to_maturity - Time to expiration
 *   num_steps        - Number of exercise opportunities
 *   type             - MCO_CALL or MCO_PUT
 *
 * Returns:
 *   American option price
 *
 * Note: American calls on non-dividend paying stocks should equal
 * European calls (early exercise is never optimal). This method
 * is primarily useful for American puts.
 */
double mco_lsm_american(mco_ctx *ctx,
                        double spot,
                        double strike,
                        double rate,
                        double volatility,
                        double time_to_maturity,
                        size_t num_steps,
                        mco_option_type type);

/*
 * Laguerre polynomial basis functions (normalized by strike).
 *
 * L0(x) = exp(-x/2)
 * L1(x) = exp(-x/2) * (1 - x)
 * L2(x) = exp(-x/2) * (1 - 2x + x²/2)
 *
 * We use the simpler non-exponential form for numerical stability:
 * L0(x) = 1
 * L1(x) = 1 - x
 * L2(x) = 1 - 2x + x²/2
 */
static inline void mco_lsm_basis(double x, double *basis)
{
    basis[0] = 1.0;
    basis[1] = 1.0 - x;
    basis[2] = 1.0 - 2.0 * x + 0.5 * x * x;
}

/*
 * Solve least squares regression: minimize ||Ax - b||²
 *
 * Uses normal equations: (A'A)x = A'b
 * Solved via Cholesky-like decomposition for small systems.
 *
 * Parameters:
 *   A      - Design matrix (n_samples × n_basis), row-major
 *   b      - Target vector (n_samples)
 *   coeffs - Output coefficients (n_basis)
 *   n_samples - Number of samples
 *   n_basis   - Number of basis functions
 *
 * Returns:
 *   0 on success, -1 if matrix is singular
 */
int mco_lsm_regress(const double *A,
                    const double *b,
                    double *coeffs,
                    size_t n_samples,
                    size_t n_basis);

#endif /* MCO_INTERNAL_METHODS_LSM_H */
