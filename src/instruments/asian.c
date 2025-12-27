/*
 * Asian Option Implementation
 */

#include "internal/instruments/asian.h"
#include "internal/models/gbm.h"
#include "internal/allocator.h"
#include "mcoptions.h"
#include <math.h>

/*============================================================================
 * Geometric Average Closed-Form
 *============================================================================*/

/*
 * For geometric Asian, the average G = (∏ S(tᵢ))^(1/n) is lognormal.
 *
 * Under risk-neutral measure:
 *   E[G] = S₀ · exp(adj_rate · T)
 *   Var[log G] = adj_vol² · T
 *
 * Where:
 *   adj_rate = (r - σ²/2) · (n+1)/(2n) + σ²/2 · (n+1)(2n+1)/(6n²)
 *   adj_vol² = σ² · (n+1)(2n+1)/(6n²)
 *
 * Simplified for large n:
 *   adj_rate ≈ (r - σ²/6) / 2
 *   adj_vol ≈ σ / √3
 */
double mco_asian_geometric_closed(double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity,
                                  size_t num_obs,
                                  mco_option_type option_type)
{
    if (spot <= 0.0 || strike <= 0.0 || time_to_maturity <= 0.0 || num_obs == 0) {
        return 0.0;
    }

    double n = (double)num_obs;
    double sigma_sq = volatility * volatility;

    /* Adjusted parameters for geometric average */
    double adj_rate = (rate - 0.5 * sigma_sq) * (n + 1.0) / (2.0 * n)
                    + sigma_sq * (n + 1.0) * (2.0 * n + 1.0) / (6.0 * n * n);

    double adj_vol_sq = sigma_sq * (n + 1.0) * (2.0 * n + 1.0) / (6.0 * n * n);
    double adj_vol = sqrt(adj_vol_sq);

    /* Use Black-Scholes with adjusted parameters */
    double sqrt_t = sqrt(time_to_maturity);
    double d1 = (log(spot / strike) + (adj_rate + 0.5 * adj_vol_sq) * time_to_maturity)
                / (adj_vol * sqrt_t);
    double d2 = d1 - adj_vol * sqrt_t;

    /* Standard normal CDF using erfc from math.h */
    double nd1 = 0.5 * erfc(-d1 * 0.7071067811865475);
    double nd2 = 0.5 * erfc(-d2 * 0.7071067811865475);

    double df = exp(-rate * time_to_maturity);

    if (option_type == MCO_CALL) {
        return spot * exp((adj_rate - rate) * time_to_maturity) * nd1 - strike * df * nd2;
    } else {
        double nmd1 = 0.5 * erfc(d1 * 0.7071067811865475);
        double nmd2 = 0.5 * erfc(d2 * 0.7071067811865475);
        return strike * df * nmd2 - spot * exp((adj_rate - rate) * time_to_maturity) * nmd1;
    }
}

/*============================================================================
 * Monte Carlo Asian Pricing
 *============================================================================*/

double mco_price_asian(mco_ctx *ctx,
                       double spot,
                       double strike,
                       double rate,
                       double volatility,
                       double time_to_maturity,
                       size_t num_obs,
                       mco_asian_type avg_type,
                       mco_asian_strike strike_type,
                       mco_option_type option_type)
{
    if (!ctx || num_obs == 0) return 0.0;

    if (spot <= 0.0 || strike <= 0.0 || volatility < 0.0 || time_to_maturity < 0.0) {
        ctx->last_error = MCO_ERR_INVALID_ARG;
        return 0.0;
    }

    uint64_t n_paths = ctx->num_simulations;

    /* Allocate path storage */
    double *path = (double *)mco_malloc((num_obs + 1) * sizeof(double));
    if (!path) {
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Initialize GBM path model */
    mco_gbm_path model;
    mco_gbm_path_init(&model, spot, rate, volatility, time_to_maturity, num_obs);

    double sum_payoff = 0.0;
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        /* Simulate path */
        mco_gbm_simulate_path(&model, &rng, path);

        /* Compute average (skip path[0] = initial spot for standard Asian) */
        double avg;
        if (avg_type == MCO_ASIAN_ARITHMETIC) {
            /* Arithmetic average */
            double sum = 0.0;
            for (size_t j = 1; j <= num_obs; ++j) {
                sum += path[j];
            }
            avg = sum / (double)num_obs;
        } else {
            /* Geometric average */
            double log_sum = 0.0;
            for (size_t j = 1; j <= num_obs; ++j) {
                log_sum += log(path[j]);
            }
            avg = exp(log_sum / (double)num_obs);
        }

        /* Compute payoff */
        double payoff;
        double terminal = path[num_obs];

        if (strike_type == MCO_ASIAN_FIXED_STRIKE) {
            /* Fixed strike: payoff based on average vs strike */
            payoff = mco_payoff(avg, strike, option_type);
        } else {
            /* Floating strike: payoff based on terminal vs average */
            if (option_type == MCO_CALL) {
                payoff = fmax(terminal - avg, 0.0);
            } else {
                payoff = fmax(avg - terminal, 0.0);
            }
        }

        sum_payoff += payoff;
    }

    mco_free(path);

    double price = model.discount * (sum_payoff / (double)n_paths);
    return price;
}

/*============================================================================
 * Public API
 *============================================================================*/

double mco_asian_call(mco_ctx *ctx,
                      double spot,
                      double strike,
                      double rate,
                      double volatility,
                      double time_to_maturity,
                      size_t num_obs)
{
    return mco_price_asian(ctx, spot, strike, rate, volatility, time_to_maturity,
                           num_obs, MCO_ASIAN_ARITHMETIC, MCO_ASIAN_FIXED_STRIKE, MCO_CALL);
}

double mco_asian_put(mco_ctx *ctx,
                     double spot,
                     double strike,
                     double rate,
                     double volatility,
                     double time_to_maturity,
                     size_t num_obs)
{
    return mco_price_asian(ctx, spot, strike, rate, volatility, time_to_maturity,
                           num_obs, MCO_ASIAN_ARITHMETIC, MCO_ASIAN_FIXED_STRIKE, MCO_PUT);
}

double mco_asian_geometric_call(mco_ctx *ctx,
                                double spot,
                                double strike,
                                double rate,
                                double volatility,
                                double time_to_maturity,
                                size_t num_obs)
{
    return mco_price_asian(ctx, spot, strike, rate, volatility, time_to_maturity,
                           num_obs, MCO_ASIAN_GEOMETRIC, MCO_ASIAN_FIXED_STRIKE, MCO_CALL);
}

double mco_asian_geometric_put(mco_ctx *ctx,
                               double spot,
                               double strike,
                               double rate,
                               double volatility,
                               double time_to_maturity,
                               size_t num_obs)
{
    return mco_price_asian(ctx, spot, strike, rate, volatility, time_to_maturity,
                           num_obs, MCO_ASIAN_GEOMETRIC, MCO_ASIAN_FIXED_STRIKE, MCO_PUT);
}
