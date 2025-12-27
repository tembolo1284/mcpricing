/*
 * American Option Pricing
 *
 * American options can be exercised at any time up to maturity.
 * Priced using Least Squares Monte Carlo (Longstaff-Schwartz).
 *
 * Key insight: Early exercise is optimal for puts when the stock
 * price is sufficiently low (time value < intrinsic value gain).
 * For calls on non-dividend paying stocks, early exercise is never
 * optimal, so American call ≈ European call.
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_AMERICAN_H
#define MCO_INTERNAL_INSTRUMENTS_AMERICAN_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"

/*
 * Price an American option using LSM.
 *
 * Parameters:
 *   ctx              - Simulation context
 *   spot             - Current spot price
 *   strike           - Strike price
 *   rate             - Risk-free rate
 *   volatility       - Volatility
 *   time_to_maturity - Time to expiration
 *   num_steps        - Number of exercise opportunities (default: 52 for weekly)
 *   type             - MCO_CALL or MCO_PUT
 *
 * Returns:
 *   American option price
 */
double mco_price_american(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time_to_maturity,
                          size_t num_steps,
                          mco_option_type type);

#endif /* MCO_INTERNAL_INSTRUMENTS_AMERICAN_H */
