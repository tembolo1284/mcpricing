/*
 * Asian Option Pricing
 *
 * Asian options have payoffs based on the average price over the life
 * of the option, rather than the terminal price.
 *
 * Types:
 *   - Arithmetic average: payoff based on (1/n)·Σ S(tᵢ)
 *   - Geometric average: payoff based on (∏ S(tᵢ))^(1/n)
 *
 * Styles:
 *   - Fixed strike: payoff = max(A - K, 0) for call
 *   - Floating strike: payoff = max(S(T) - A, 0) for call
 *
 * Properties:
 *   - Lower volatility than vanilla (averaging smooths out extremes)
 *   - Cheaper than vanilla options
 *   - Geometric average has closed-form solution
 *   - Arithmetic average requires Monte Carlo
 *
 * This implementation:
 *   - Fixed strike arithmetic average (most common)
 *   - Discrete averaging at num_obs observation points
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_ASIAN_H
#define MCO_INTERNAL_INSTRUMENTS_ASIAN_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"
#include <stddef.h>

/*
 * Asian averaging type
 */
typedef enum {
    MCO_ASIAN_ARITHMETIC = 0,
    MCO_ASIAN_GEOMETRIC  = 1
} mco_asian_type;

/*
 * Asian strike type
 */
typedef enum {
    MCO_ASIAN_FIXED_STRIKE    = 0,  /* Payoff: max(A - K, 0) */
    MCO_ASIAN_FLOATING_STRIKE = 1   /* Payoff: max(S(T) - A, 0) */
} mco_asian_strike;

/*
 * Price an Asian option using Monte Carlo.
 *
 * Parameters:
 *   ctx              - Simulation context
 *   spot             - Current spot price
 *   strike           - Strike price (for fixed strike)
 *   rate             - Risk-free rate
 *   volatility       - Volatility
 *   time_to_maturity - Time to expiration
 *   num_obs          - Number of averaging observations
 *   avg_type         - Arithmetic or geometric
 *   strike_type      - Fixed or floating strike
 *   option_type      - Call or put
 *
 * Returns:
 *   Asian option price
 */
double mco_price_asian(mco_ctx *ctx,
                       double spot,
                       double strike,
                       double rate,
                       double volatility,
                       double time_to_maturity,
                       size_t num_obs,
                       mco_asian_type avg_type,
                       mco_asian_strike strike_type,
                       mco_option_type option_type);

/*
 * Closed-form geometric Asian option price (for validation)
 *
 * The geometric average of lognormal prices is lognormal,
 * allowing closed-form pricing via adjusted Black-Scholes.
 */
double mco_asian_geometric_closed(double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity,
                                  size_t num_obs,
                                  mco_option_type option_type);

#endif /* MCO_INTERNAL_INSTRUMENTS_ASIAN_H */
