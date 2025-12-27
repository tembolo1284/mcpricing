/*
 * Digital (Binary) Options Implementation
 */

#include "internal/instruments/digital.h"
#include "internal/models/gbm.h"
#include "mcoptions.h"
#include <math.h>

static double norm_cdf(double x)
{
    return 0.5 * erfc(-x * 0.7071067811865475);
}

/*============================================================================
 * Monte Carlo Digital Pricing
 *============================================================================*/

double mco_price_digital(mco_ctx *ctx,
                         double spot,
                         double strike,
                         double payout,
                         double rate,
                         double volatility,
                         double time,
                         mco_digital_type digital_type,
                         mco_option_type option_type)
{
    if (!ctx) return 0.0;

    uint64_t n_paths = ctx->num_simulations;

    /* Initialize GBM model */
    mco_gbm model;
    mco_gbm_init(&model, spot, rate, volatility, time);

    double sum_payoff = 0.0;
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        double s_T = mco_gbm_simulate(&model, &rng);

        int itm = (option_type == MCO_CALL) ? (s_T > strike) : (s_T < strike);

        double payoff = 0.0;
        if (itm) {
            if (digital_type == MCO_DIGITAL_CASH) {
                payoff = payout;
            } else {
                payoff = s_T;
            }
        }

        sum_payoff += payoff;
    }

    return model.discount * (sum_payoff / (double)n_paths);
}

/*============================================================================
 * Analytical Digital Formulas
 *============================================================================*/

double mco_digital_cash_call(double spot, double strike, double payout,
                              double rate, double vol, double time)
{
    if (time <= 0.0) {
        return (spot > strike) ? payout : 0.0;
    }
    if (vol <= 0.0) {
        return (spot > strike * exp(-rate * time)) ? payout * exp(-rate * time) : 0.0;
    }

    double sqrt_t = sqrt(time);
    double d2 = (log(spot / strike) + (rate - 0.5 * vol * vol) * time) / (vol * sqrt_t);

    return payout * exp(-rate * time) * norm_cdf(d2);
}

double mco_digital_cash_put(double spot, double strike, double payout,
                             double rate, double vol, double time)
{
    if (time <= 0.0) {
        return (spot < strike) ? payout : 0.0;
    }
    if (vol <= 0.0) {
        return (spot < strike * exp(-rate * time)) ? payout * exp(-rate * time) : 0.0;
    }

    double sqrt_t = sqrt(time);
    double d2 = (log(spot / strike) + (rate - 0.5 * vol * vol) * time) / (vol * sqrt_t);

    return payout * exp(-rate * time) * norm_cdf(-d2);
}

double mco_digital_asset_call(double spot, double strike,
                               double rate, double vol, double time)
{
    if (time <= 0.0) {
        return (spot > strike) ? spot : 0.0;
    }
    if (vol <= 0.0) {
        return (spot > strike * exp(-rate * time)) ? spot : 0.0;
    }

    double sqrt_t = sqrt(time);
    double d1 = (log(spot / strike) + (rate + 0.5 * vol * vol) * time) / (vol * sqrt_t);

    return spot * norm_cdf(d1);
}

double mco_digital_asset_put(double spot, double strike,
                              double rate, double vol, double time)
{
    if (time <= 0.0) {
        return (spot < strike) ? spot : 0.0;
    }
    if (vol <= 0.0) {
        return (spot < strike * exp(-rate * time)) ? spot : 0.0;
    }

    double sqrt_t = sqrt(time);
    double d1 = (log(spot / strike) + (rate + 0.5 * vol * vol) * time) / (vol * sqrt_t);

    return spot * norm_cdf(-d1);
}

/*============================================================================
 * Public API
 *============================================================================*/

double mco_digital_call(mco_ctx *ctx, double spot, double strike, double payout,
                        double rate, double vol, double time, int cash_or_nothing)
{
    mco_digital_type type = cash_or_nothing ? MCO_DIGITAL_CASH : MCO_DIGITAL_ASSET;
    return mco_price_digital(ctx, spot, strike, payout, rate, vol, time, type, MCO_CALL);
}

double mco_digital_put(mco_ctx *ctx, double spot, double strike, double payout,
                       double rate, double vol, double time, int cash_or_nothing)
{
    mco_digital_type type = cash_or_nothing ? MCO_DIGITAL_CASH : MCO_DIGITAL_ASSET;
    return mco_price_digital(ctx, spot, strike, payout, rate, vol, time, type, MCO_PUT);
}
