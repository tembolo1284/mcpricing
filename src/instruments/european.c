/*
 * European Option Pricing - Monte Carlo Implementation
 *
 * European options can only be exercised at maturity, making them
 * the simplest case. We only need the terminal spot price, not the
 * full path - this allows for significant optimization.
 *
 * For single-threaded execution, we run the simulation directly.
 * For multi-threaded, we delegate to the thread pool.
 */

#include "internal/instruments/european.h"
#include "internal/models/gbm.h"
#include "internal/variance_reduction/antithetic.h"
#include "internal/methods/thread_pool.h"
#include "internal/allocator.h"
#include "mcoptions.h"

/*============================================================================
 * Single-Threaded Implementation
 *============================================================================*/

/*
 * Price European option without antithetic variates.
 */
static double price_european_basic(mco_ctx *ctx,
                                   double spot,
                                   double strike,
                                   double rate,
                                   double volatility,
                                   double time_to_maturity,
                                   mco_option_type type)
{
    mco_gbm model;
    mco_gbm_init(&model, spot, rate, volatility, time_to_maturity);

    double sum = 0.0;
    uint64_t n = ctx->num_simulations;

    for (uint64_t i = 0; i < n; ++i) {
        double s_t = mco_gbm_simulate(&model, &ctx->rng);
        sum += mco_payoff(s_t, strike, type);
    }

    return model.discount * (sum / (double)n);
}

/*
 * Price European option with antithetic variates.
 */
static double price_european_antithetic(mco_ctx *ctx,
                                        double spot,
                                        double strike,
                                        double rate,
                                        double volatility,
                                        double time_to_maturity,
                                        mco_option_type type)
{
    mco_gbm model;
    mco_gbm_init(&model, spot, rate, volatility, time_to_maturity);

    return mco_antithetic_european(&model, &ctx->rng, strike, type, 
                                   ctx->num_simulations);
}

/*============================================================================
 * Main Entry Point
 *============================================================================*/

/*
 * Price a European option using Monte Carlo simulation.
 *
 * Automatically selects:
 *   - Single vs multi-threaded based on ctx->num_threads
 *   - Antithetic variates based on ctx->antithetic_enabled
 */
double mco_price_european(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time_to_maturity,
                          mco_option_type type)
{
    if (!ctx) return 0.0;

    /* Validate inputs */
    if (spot <= 0.0 || strike <= 0.0 || volatility < 0.0 || time_to_maturity < 0.0) {
        ctx->last_error = MCO_ERR_INVALID_ARG;
        return 0.0;
    }

    /* Multi-threaded path */
    if (ctx->num_threads > 1) {
        return mco_parallel_european(ctx, spot, strike, rate, volatility,
                                     time_to_maturity, type);
    }

    /* Single-threaded path */
    if (ctx->antithetic_enabled) {
        return price_european_antithetic(ctx, spot, strike, rate, volatility,
                                         time_to_maturity, type);
    } else {
        return price_european_basic(ctx, spot, strike, rate, volatility,
                                    time_to_maturity, type);
    }
}

/*============================================================================
 * Public API Wrappers
 *============================================================================*/

double mco_european_call(mco_ctx *ctx,
                         double spot,
                         double strike,
                         double rate,
                         double volatility,
                         double time_to_maturity)
{
    return mco_price_european(ctx, spot, strike, rate, volatility,
                              time_to_maturity, MCO_CALL);
}

double mco_european_put(mco_ctx *ctx,
                        double spot,
                        double strike,
                        double rate,
                        double volatility,
                        double time_to_maturity)
{
    return mco_price_european(ctx, spot, strike, rate, volatility,
                              time_to_maturity, MCO_PUT);
}
