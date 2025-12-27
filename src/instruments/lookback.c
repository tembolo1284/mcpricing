/*
 * Lookback Options Implementation
 */

#include "internal/instruments/lookback.h"
#include "internal/models/gbm.h"
#include "internal/allocator.h"
#include "mcoptions.h"
#include <math.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double norm_cdf(double x)
{
    return 0.5 * erfc(-x * 0.7071067811865475);
}

/*============================================================================
 * Monte Carlo Lookback Pricing
 *============================================================================*/

double mco_price_lookback(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time,
                          size_t num_steps,
                          mco_lookback_strike strike_type,
                          mco_option_type option_type)
{
    if (!ctx || num_steps == 0) return 0.0;

    uint64_t n_paths = ctx->num_simulations;

    /* Allocate path storage */
    double *path = (double *)mco_malloc((num_steps + 1) * sizeof(double));
    if (!path) {
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Initialize GBM path model */
    mco_gbm_path model;
    mco_gbm_path_init(&model, spot, rate, volatility, time, num_steps);

    double sum_payoff = 0.0;
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        /* Simulate path */
        mco_gbm_simulate_path(&model, &rng, path);

        /* Find min and max */
        double path_min = path[0];
        double path_max = path[0];

        for (size_t j = 1; j <= num_steps; ++j) {
            if (path[j] < path_min) path_min = path[j];
            if (path[j] > path_max) path_max = path[j];
        }

        double terminal = path[num_steps];
        double payoff = 0.0;

        if (strike_type == MCO_LOOKBACK_FLOATING) {
            /* Floating strike */
            if (option_type == MCO_CALL) {
                /* Buy at minimum: S(T) - min(S) */
                payoff = terminal - path_min;
            } else {
                /* Sell at maximum: max(S) - S(T) */
                payoff = path_max - terminal;
            }
        } else {
            /* Fixed strike */
            if (option_type == MCO_CALL) {
                /* max(max(S) - K, 0) */
                payoff = fmax(path_max - strike, 0.0);
            } else {
                /* max(K - min(S), 0) */
                payoff = fmax(strike - path_min, 0.0);
            }
        }

        sum_payoff += payoff;
    }

    mco_free(path);

    return model.discount * (sum_payoff / (double)n_paths);
}

/*============================================================================
 * Analytical Lookback Formulas
 *============================================================================*/

/*
 * Floating strike lookback call: E[S(T) - min(S)]
 *
 * Goldman, Sosin, Gatto (1979):
 *   C = S·N(a₁) - S·(σ²/2r)·N(-a₁) - S·e^(-rT)·[N(a₂) - (σ²/2r)·e^(a₃)·N(-a₃)]
 *
 * Simplified form for r ≠ 0
 */
double mco_lookback_floating_call(double spot, double rate, double vol, double time)
{
    if (spot <= 0.0 || time <= 0.0 || vol <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double vol_sqrt_t = vol * sqrt_t;

    double a1 = (rate / vol + 0.5 * vol) * sqrt_t;
    double a2 = a1 - vol_sqrt_t;

    double df = exp(-rate * time);

    if (fabs(rate) < 1e-10) {
        /* r ≈ 0: simplified formula */
        return spot * (norm_cdf(a1) - norm_cdf(-a1)) + spot * vol * sqrt_t * 0.7978845608;  /* sqrt(2/π) */
    }

    double vol_sq_over_2r = vol * vol / (2.0 * rate);

    double term1 = spot * norm_cdf(a1);
    double term2 = -spot * vol_sq_over_2r * norm_cdf(-a1);
    double term3 = -spot * df * norm_cdf(a2);
    double term4 = spot * df * vol_sq_over_2r * exp(2.0 * rate * time / (vol * vol) * a1 * vol / sqrt_t) * norm_cdf(-a1);

    /* More accurate formula */
    double mu = rate - 0.5 * vol * vol;
    double d1 = (mu * time + vol_sqrt_t * vol_sqrt_t) / vol_sqrt_t;
    double d2 = mu * time / vol_sqrt_t;

    return spot * norm_cdf(d1) - spot * df * norm_cdf(d2)
         + spot * vol_sq_over_2r * (norm_cdf(-d1) - df * exp(2.0 * mu / vol / vol * log(1.0)) * norm_cdf(-d2));
}

/*
 * Floating strike lookback put: E[max(S) - S(T)]
 */
double mco_lookback_floating_put(double spot, double rate, double vol, double time)
{
    if (spot <= 0.0 || time <= 0.0 || vol <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double vol_sqrt_t = vol * sqrt_t;
    double df = exp(-rate * time);

    double a1 = (rate / vol + 0.5 * vol) * sqrt_t;
    double a2 = a1 - vol_sqrt_t;

    if (fabs(rate) < 1e-10) {
        return spot * (norm_cdf(a1) - norm_cdf(-a1)) + spot * vol * sqrt_t * 0.7978845608;
    }

    double vol_sq_over_2r = vol * vol / (2.0 * rate);

    /* Use symmetry: put payoff is symmetric to call with sign changes */
    double term1 = -spot * norm_cdf(-a1);
    double term2 = spot * vol_sq_over_2r * norm_cdf(a1);
    double term3 = spot * df * norm_cdf(-a2);
    double term4 = -spot * df * vol_sq_over_2r * norm_cdf(a2);

    return -(term1 + term2 + term3 + term4);
}

/*
 * Fixed strike lookback call: E[max(max(S) - K, 0)]
 */
double mco_lookback_fixed_call(double spot, double strike, double rate, double vol, double time)
{
    if (spot <= 0.0 || strike <= 0.0 || time <= 0.0 || vol <= 0.0) {
        return fmax(spot - strike, 0.0);
    }

    /* For S₀ ≥ K, this is approximately the floating call + adjustment */
    /* For S₀ < K, more complex formula needed */

    double sqrt_t = sqrt(time);
    double vol_sqrt_t = vol * sqrt_t;
    double df = exp(-rate * time);

    double d1 = (log(spot / strike) + (rate + 0.5 * vol * vol) * time) / vol_sqrt_t;
    double d2 = d1 - vol_sqrt_t;

    /* Vanilla call component */
    double vanilla = spot * norm_cdf(d1) - strike * df * norm_cdf(d2);

    /* Lookback premium: value of seeing the maximum */
    double premium = mco_lookback_floating_call(spot, rate, vol, time) - spot + spot * df;

    return vanilla + fmax(premium, 0.0);
}

/*
 * Fixed strike lookback put: E[max(K - min(S), 0)]
 */
double mco_lookback_fixed_put(double spot, double strike, double rate, double vol, double time)
{
    if (spot <= 0.0 || strike <= 0.0 || time <= 0.0 || vol <= 0.0) {
        return fmax(strike - spot, 0.0);
    }

    double sqrt_t = sqrt(time);
    double vol_sqrt_t = vol * sqrt_t;
    double df = exp(-rate * time);

    double d1 = (log(spot / strike) + (rate + 0.5 * vol * vol) * time) / vol_sqrt_t;
    double d2 = d1 - vol_sqrt_t;

    /* Vanilla put component */
    double vanilla = strike * df * norm_cdf(-d2) - spot * norm_cdf(-d1);

    /* Lookback premium */
    double premium = mco_lookback_floating_put(spot, rate, vol, time) - spot * df + spot;

    return vanilla + fmax(premium, 0.0);
}

/*============================================================================
 * Public API
 *============================================================================*/

double mco_lookback_call(mco_ctx *ctx, double spot, double strike,
                         double rate, double vol, double time, size_t steps,
                         int floating_strike)
{
    mco_lookback_strike type = floating_strike ? MCO_LOOKBACK_FLOATING : MCO_LOOKBACK_FIXED;
    return mco_price_lookback(ctx, spot, strike, rate, vol, time, steps, type, MCO_CALL);
}

double mco_lookback_put(mco_ctx *ctx, double spot, double strike,
                        double rate, double vol, double time, size_t steps,
                        int floating_strike)
{
    mco_lookback_strike type = floating_strike ? MCO_LOOKBACK_FLOATING : MCO_LOOKBACK_FIXED;
    return mco_price_lookback(ctx, spot, strike, rate, vol, time, steps, type, MCO_PUT);
}
