/*
 * Lookback Options
 *
 * Lookback options have payoffs that depend on the extremum (min or max)
 * of the underlying price over the life of the option.
 *
 * Types:
 *   Floating Strike:
 *     - Call: S(T) - min(S)  (buy at the lowest price)
 *     - Put:  max(S) - S(T)  (sell at the highest price)
 *
 *   Fixed Strike:
 *     - Call: max(max(S) - K, 0)
 *     - Put:  max(K - min(S), 0)
 *
 * Properties:
 *   - Always ITM at expiry (for floating strike)
 *   - More expensive than vanilla options
 *   - Analytical formulas exist (Goldman, Sosin, Gatto 1979)
 *
 * Reference:
 *   Goldman, M.B., Sosin, H.B., Gatto, M.A. (1979)
 *   "Path Dependent Options: Buy at the Low, Sell at the High"
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_LOOKBACK_H
#define MCO_INTERNAL_INSTRUMENTS_LOOKBACK_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"
#include <stddef.h>

/*
 * Lookback strike type
 */
typedef enum {
    MCO_LOOKBACK_FLOATING = 0,  /* Strike is min or max */
    MCO_LOOKBACK_FIXED    = 1   /* Strike is fixed */
} mco_lookback_strike;

/*
 * Price a lookback option using Monte Carlo
 *
 * Parameters:
 *   ctx          - Simulation context
 *   spot         - Current spot price
 *   strike       - Strike price (for fixed strike only)
 *   rate         - Risk-free rate
 *   volatility   - Volatility
 *   time         - Time to maturity
 *   num_steps    - Number of observation points
 *   strike_type  - Floating or fixed strike
 *   option_type  - Call or put
 *
 * Returns:
 *   Lookback option price
 */
double mco_price_lookback(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time,
                          size_t num_steps,
                          mco_lookback_strike strike_type,
                          mco_option_type option_type);

/*
 * Analytical lookback formulas (floating strike, continuous monitoring)
 * Goldman, Sosin, Gatto (1979)
 */
double mco_lookback_floating_call(double spot, double rate, double vol, double time);
double mco_lookback_floating_put(double spot, double rate, double vol, double time);

/*
 * Fixed strike lookback (Conze & Viswanathan 1991)
 */
double mco_lookback_fixed_call(double spot, double strike, double rate, double vol, double time);
double mco_lookback_fixed_put(double spot, double strike, double rate, double vol, double time);

#endif /* MCO_INTERNAL_INSTRUMENTS_LOOKBACK_H */
