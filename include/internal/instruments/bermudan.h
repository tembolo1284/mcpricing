/*
 * Bermudan Option Pricing
 *
 * Bermudan options can be exercised on specific dates before maturity
 * (a hybrid between European and American options).
 *
 * Common examples:
 *   - Monthly exercise opportunities
 *   - Quarterly exercise
 *   - Exercise on coupon dates (for swaptions)
 *
 * Pricing:
 *   Uses Least Squares Monte Carlo (LSM) like American options,
 *   but only evaluates exercise at specified dates.
 *
 * Value relationship:
 *   European ≤ Bermudan ≤ American
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_BERMUDAN_H
#define MCO_INTERNAL_INSTRUMENTS_BERMUDAN_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"
#include <stddef.h>

/*
 * Price a Bermudan option using LSM.
 *
 * Parameters:
 *   ctx              - Simulation context
 *   spot             - Current spot price
 *   strike           - Strike price
 *   rate             - Risk-free rate
 *   volatility       - Volatility
 *   time_to_maturity - Time to final expiration
 *   exercise_times   - Array of exercise times (as fractions of T)
 *                      e.g., {0.25, 0.5, 0.75, 1.0} for quarterly
 *   num_exercise     - Number of exercise opportunities
 *   type             - MCO_CALL or MCO_PUT
 *
 * Returns:
 *   Bermudan option price
 *
 * Note: exercise_times should be sorted in ascending order and
 * should include the final maturity (1.0).
 */
double mco_price_bermudan(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time_to_maturity,
                          const double *exercise_times,
                          size_t num_exercise,
                          mco_option_type type);

/*
 * Price a Bermudan option with evenly spaced exercise dates.
 *
 * Convenience function that creates exercise_times automatically.
 * E.g., num_exercise=4 creates {0.25, 0.5, 0.75, 1.0}
 */
double mco_price_bermudan_uniform(mco_ctx *ctx,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity,
                                  size_t num_exercise,
                                  mco_option_type type);

#endif /* MCO_INTERNAL_INSTRUMENTS_BERMUDAN_H */
