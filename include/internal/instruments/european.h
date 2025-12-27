/*
 * European option pricing
 *
 * European options can only be exercised at maturity.
 * This is the simplest case and serves as the foundation.
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_EUROPEAN_H
#define MCO_INTERNAL_INSTRUMENTS_EUROPEAN_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"

/*
 * Price a European option using Monte Carlo simulation.
 *
 * Parameters:
 *   ctx              - Simulation context (contains num_simulations, threads, etc.)
 *   spot             - Current underlying price
 *   strike           - Strike price
 *   rate             - Risk-free interest rate (annualized, continuous)
 *   volatility       - Volatility (annualized)
 *   time_to_maturity - Time to expiration in years
 *   type             - MCO_CALL or MCO_PUT
 *
 * Returns:
 *   Discounted expected payoff (option price)
 */
double mco_price_european(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time_to_maturity,
                          mco_option_type type);

#endif /* MCO_INTERNAL_INSTRUMENTS_EUROPEAN_H */
