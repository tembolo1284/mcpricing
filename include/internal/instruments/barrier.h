/*
 * Barrier Options
 *
 * Barrier options are path-dependent options that either come into
 * existence (knock-in) or cease to exist (knock-out) when the
 * underlying crosses a barrier level.
 *
 * Types:
 *   - Up-and-In:   Activated when S rises above barrier H
 *   - Up-and-Out:  Cancelled when S rises above barrier H
 *   - Down-and-In: Activated when S falls below barrier H
 *   - Down-and-Out: Cancelled when S falls below barrier H
 *
 * Parity relations:
 *   Knock-In + Knock-Out = Vanilla
 *
 * Rebate:
 *   Some barrier options pay a rebate when knocked out.
 *
 * Pricing considerations:
 *   - Continuous vs discrete monitoring
 *   - Brownian bridge for better barrier detection
 *   - Analytical formulas exist for continuous monitoring
 *
 * Reference:
 *   Merton (1973), Reiner & Rubinstein (1991)
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_BARRIER_H
#define MCO_INTERNAL_INSTRUMENTS_BARRIER_H

#include "mcoptions.h"
#include "internal/context.h"
#include "internal/instruments/payoff.h"
#include <stddef.h>

/*
 * Note: mco_barrier_style is defined in mcoptions.h
 */

/*
 * Check if barrier was hit during a path
 *
 * For continuous monitoring, we use Brownian bridge probability:
 * P(min(S) < H | S(0), S(dt)) for down barriers
 * P(max(S) > H | S(0), S(dt)) for up barriers
 */

/*
 * Price a barrier option using Monte Carlo
 *
 * Parameters:
 *   ctx          - Simulation context
 *   spot         - Current spot price
 *   strike       - Strike price
 *   barrier      - Barrier level H
 *   rebate       - Rebate paid if knocked out (0 for none)
 *   rate         - Risk-free rate
 *   volatility   - Volatility
 *   time         - Time to maturity
 *   num_steps    - Number of monitoring points
 *   barrier_type - Up/Down, In/Out
 *   option_type  - Call or Put
 *
 * Returns:
 *   Barrier option price
 */
double mco_price_barrier(mco_ctx *ctx,
                         double spot,
                         double strike,
                         double barrier,
                         double rebate,
                         double rate,
                         double volatility,
                         double time,
                         size_t num_steps,
                         mco_barrier_style barrier_type,
                         mco_option_type option_type);

/*
 * Analytical barrier formulas (continuous monitoring)
 * From Reiner & Rubinstein (1991)
 */
double mco_barrier_down_out_call(double spot, double strike, double barrier,
                                  double rebate, double rate, double vol, double time);

double mco_barrier_down_in_call(double spot, double strike, double barrier,
                                 double rebate, double rate, double vol, double time);

double mco_barrier_up_out_call(double spot, double strike, double barrier,
                                double rebate, double rate, double vol, double time);

double mco_barrier_up_in_call(double spot, double strike, double barrier,
                               double rebate, double rate, double vol, double time);

double mco_barrier_down_out_put(double spot, double strike, double barrier,
                                 double rebate, double rate, double vol, double time);

double mco_barrier_down_in_put(double spot, double strike, double barrier,
                                double rebate, double rate, double vol, double time);

double mco_barrier_up_out_put(double spot, double strike, double barrier,
                               double rebate, double rate, double vol, double time);

double mco_barrier_up_in_put(double spot, double strike, double barrier,
                              double rebate, double rate, double vol, double time);

#endif /* MCO_INTERNAL_INSTRUMENTS_BARRIER_H */
