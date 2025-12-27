/*
 * American Option Pricing Implementation
 */

#include "internal/instruments/american.h"
#include "internal/methods/lsm.h"
#include "mcoptions.h"

/*
 * Default number of exercise opportunities if not specified
 */
#define MCO_DEFAULT_AMERICAN_STEPS 52  /* Weekly exercise */

double mco_price_american(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time_to_maturity,
                          size_t num_steps,
                          mco_option_type type)
{
    if (!ctx) return 0.0;

    /* Validate inputs */
    if (spot <= 0.0 || strike <= 0.0 || volatility < 0.0 || time_to_maturity < 0.0) {
        ctx->last_error = MCO_ERR_INVALID_ARG;
        return 0.0;
    }

    /* Use default steps if not specified */
    if (num_steps == 0) {
        num_steps = MCO_DEFAULT_AMERICAN_STEPS;
    }

    return mco_lsm_american(ctx, spot, strike, rate, volatility,
                            time_to_maturity, num_steps, type);
}

/*============================================================================
 * Public API Wrappers
 *============================================================================*/

double mco_american_call(mco_ctx *ctx,
                         double spot,
                         double strike,
                         double rate,
                         double volatility,
                         double time_to_maturity,
                         size_t num_steps)
{
    return mco_price_american(ctx, spot, strike, rate, volatility,
                              time_to_maturity, num_steps, MCO_CALL);
}

double mco_american_put(mco_ctx *ctx,
                        double spot,
                        double strike,
                        double rate,
                        double volatility,
                        double time_to_maturity,
                        size_t num_steps)
{
    return mco_price_american(ctx, spot, strike, rate, volatility,
                              time_to_maturity, num_steps, MCO_PUT);
}
