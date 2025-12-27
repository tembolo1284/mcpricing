/*
 * Barrier Options Implementation
 *
 * Uses Brownian bridge for continuous barrier approximation.
 */

#include "internal/instruments/barrier.h"
#include "internal/models/gbm.h"
#include "internal/allocator.h"
#include "mcoptions.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*============================================================================
 * Brownian Bridge Barrier Probability
 *============================================================================*/

/*
 * Probability that a Brownian bridge hits a barrier.
 *
 * Given S(t) and S(t+dt), probability that min/max crossed barrier H.
 *
 * For down barrier (min < H):
 *   P = exp(-2 * log(S(t)/H) * log(S(t+dt)/H) / (σ²dt))
 *
 * For up barrier (max > H):
 *   P = exp(-2 * log(H/S(t)) * log(H/S(t+dt)) / (σ²dt))
 *
 * Only applies when S(t) and S(t+dt) are on same side of barrier.
 */
static inline double bridge_hit_prob(double s1, double s2, double h,
                                      double vol, double dt, int is_up)
{
    if (is_up) {
        /* Up barrier: check if max > H */
        if (s1 >= h || s2 >= h) return 1.0;  /* Already hit */
        if (s1 <= 0.0 || s2 <= 0.0) return 0.0;

        double log1 = log(h / s1);
        double log2 = log(h / s2);
        if (log1 <= 0.0 || log2 <= 0.0) return 1.0;

        double var = vol * vol * dt;
        return exp(-2.0 * log1 * log2 / var);
    } else {
        /* Down barrier: check if min < H */
        if (s1 <= h || s2 <= h) return 1.0;  /* Already hit */
        if (s1 <= 0.0 || s2 <= 0.0) return 0.0;

        double log1 = log(s1 / h);
        double log2 = log(s2 / h);
        if (log1 <= 0.0 || log2 <= 0.0) return 1.0;

        double var = vol * vol * dt;
        return exp(-2.0 * log1 * log2 / var);
    }
}

/*============================================================================
 * Monte Carlo Barrier Pricing
 *============================================================================*/

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

    double dt = time / num_steps;
    int is_up = (barrier_type == MCO_BARRIER_UP_IN || barrier_type == MCO_BARRIER_UP_OUT);
    int is_knock_in = (barrier_type == MCO_BARRIER_DOWN_IN || barrier_type == MCO_BARRIER_UP_IN);

    double sum_payoff = 0.0;
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        /* Simulate path */
        mco_gbm_simulate_path(&model, &rng, path);

        /* Check barrier */
        int barrier_hit = 0;

        for (size_t j = 0; j < num_steps; ++j) {
            double s1 = path[j];
            double s2 = path[j + 1];

            /* Discrete check */
            if (is_up) {
                if (s1 >= barrier || s2 >= barrier) {
                    barrier_hit = 1;
                    break;
                }
            } else {
                if (s1 <= barrier || s2 <= barrier) {
                    barrier_hit = 1;
                    break;
                }
            }

            /* Brownian bridge probability for continuous approximation */
            double p_hit = bridge_hit_prob(s1, s2, barrier, volatility, dt, is_up);
            if (mco_rng_uniform(&rng) < p_hit) {
                barrier_hit = 1;
                break;
            }
        }

        /* Compute payoff based on barrier type */
        double payoff = 0.0;
        double terminal = path[num_steps];

        if (is_knock_in) {
            /* Knock-in: pay if barrier was hit */
            if (barrier_hit) {
                payoff = mco_payoff(terminal, strike, option_type);
            }
            /* else payoff = 0 (option never activated) */
        } else {
            /* Knock-out: pay if barrier was NOT hit */
            if (!barrier_hit) {
                payoff = mco_payoff(terminal, strike, option_type);
            } else {
                /* Pay rebate (if any) */
                payoff = rebate;
            }
        }

        sum_payoff += payoff;
    }

    mco_free(path);

    return model.discount * (sum_payoff / (double)n_paths);
}

/*============================================================================
 * Analytical Barrier Formulas (Continuous Monitoring)
 *============================================================================*/

static double norm_cdf(double x)
{
    return 0.5 * erfc(-x * 0.7071067811865475);
}

/*
 * Helper functions for barrier formulas
 */
static double barrier_A(double spot, double strike, double rate, double vol, double time, double phi)
{
    double sqrt_t = sqrt(time);
    double d1 = (log(spot / strike) + (rate + 0.5 * vol * vol) * time) / (vol * sqrt_t);
    double d2 = d1 - vol * sqrt_t;
    return phi * spot * norm_cdf(phi * d1) - phi * strike * exp(-rate * time) * norm_cdf(phi * d2);
}

static double barrier_B(double spot, double strike, double barrier, double rate, double vol, double time, double phi, double eta)
{
    double sqrt_t = sqrt(time);
    double mu = (rate - 0.5 * vol * vol) / (vol * vol);
    double lambda = sqrt(mu * mu + 2.0 * rate / (vol * vol));
    (void) lambda;
    double x1 = log(spot / barrier) / (vol * sqrt_t) + (1.0 + mu) * vol * sqrt_t;
    double y1 = log(barrier / spot) / (vol * sqrt_t) + (1.0 + mu) * vol * sqrt_t;

    double pow_term = pow(barrier / spot, 2.0 * mu);

    double term1 = phi * spot * norm_cdf(phi * x1);
    double term2 = -phi * strike * exp(-rate * time) * norm_cdf(phi * x1 - phi * vol * sqrt_t);
    double term3 = -phi * spot * pow_term * norm_cdf(eta * y1);
    double term4 = phi * strike * exp(-rate * time) * pow_term * norm_cdf(eta * y1 - eta * vol * sqrt_t);

    return term1 + term2 + term3 + term4;
}

/* Down-and-Out Call: S > H, K > H */
double mco_barrier_down_out_call(double spot, double strike, double barrier,
                                  double rebate, double rate, double vol, double time)
{
    if (spot <= barrier) return rebate * exp(-rate * time);
    if (strike <= barrier) {
        /* In-the-money path, use simpler formula */
        double vanilla = mco_black_scholes_call(spot, strike, rate, vol, time);
        double in_price = mco_barrier_down_in_call(spot, strike, barrier, 0, rate, vol, time);
        return vanilla - in_price + rebate * exp(-rate * time);
    }

    /* Standard case: K > H */
    double sqrt_t = sqrt(time);
    double mu = (rate - 0.5 * vol * vol) / (vol * vol);

    double x1 = log(spot / strike) / (vol * sqrt_t) + (1.0 + mu) * vol * sqrt_t;
    double y1 = log(barrier * barrier / (spot * strike)) / (vol * sqrt_t) + (1.0 + mu) * vol * sqrt_t;

    double pow_term = pow(barrier / spot, 2.0 * mu);

    double call = spot * norm_cdf(x1) - strike * exp(-rate * time) * norm_cdf(x1 - vol * sqrt_t);
    double adjustment = spot * pow_term * norm_cdf(y1) - strike * exp(-rate * time) * pow_term * norm_cdf(y1 - vol * sqrt_t);

    return call - adjustment;
}

double mco_barrier_down_in_call(double spot, double strike, double barrier,
                                 double rebate, double rate, double vol, double time)
{
    (void)rebate;
    if (spot <= barrier) {
        return mco_black_scholes_call(spot, strike, rate, vol, time);
    }

    double vanilla = mco_black_scholes_call(spot, strike, rate, vol, time);
    double out = mco_barrier_down_out_call(spot, strike, barrier, 0, rate, vol, time);
    return vanilla - out;
}

double mco_barrier_up_out_call(double spot, double strike, double barrier,
                                double rebate, double rate, double vol, double time)
{
    if (spot >= barrier) return rebate * exp(-rate * time);
    if (barrier <= strike) return 0.0;  /* Never ITM before knockout */

    double vanilla = mco_black_scholes_call(spot, strike, rate, vol, time);
    double in_price = mco_barrier_up_in_call(spot, strike, barrier, 0, rate, vol, time);
    return vanilla - in_price;
}

double mco_barrier_up_in_call(double spot, double strike, double barrier,
                               double rebate, double rate, double vol, double time)
{
    (void) rebate;
    if (spot >= barrier) {
        return mco_black_scholes_call(spot, strike, rate, vol, time);
    }

    double sqrt_t = sqrt(time);
    double mu = (rate - 0.5 * vol * vol) / (vol * vol);
    double pow_term = pow(barrier / spot, 2.0 * mu);

    double y1 = log(barrier / spot) / (vol * sqrt_t) + (1.0 + mu) * vol * sqrt_t;

    return spot * pow_term * norm_cdf(y1) - strike * exp(-rate * time) * pow_term * norm_cdf(y1 - vol * sqrt_t);
}

/* Put versions using put-call parity-like relations */
double mco_barrier_down_out_put(double spot, double strike, double barrier,
                                 double rebate, double rate, double vol, double time)
{
    (void) rebate;
    if (spot <= barrier) return rebate * exp(-rate * time);

    double vanilla = mco_black_scholes_put(spot, strike, rate, vol, time);
    double in_price = mco_barrier_down_in_put(spot, strike, barrier, 0, rate, vol, time);
    return vanilla - in_price;
}

double mco_barrier_down_in_put(double spot, double strike, double barrier,
                                double rebate, double rate, double vol, double time)
{
    (void) rebate;
    if (spot <= barrier) {
        return mco_black_scholes_put(spot, strike, rate, vol, time);
    }

    double sqrt_t = sqrt(time);
    double mu = (rate - 0.5 * vol * vol) / (vol * vol);
    double pow_term = pow(barrier / spot, 2.0 * mu);

    double y1 = log(barrier / spot) / (vol * sqrt_t) + (1.0 + mu) * vol * sqrt_t;
    double y2 = log(barrier / spot) / (vol * sqrt_t) + mu * vol * sqrt_t;

    (void) y2;

    return -spot * pow_term * norm_cdf(-y1) + strike * exp(-rate * time) * pow_term * norm_cdf(-y1 + vol * sqrt_t);
}

double mco_barrier_up_out_put(double spot, double strike, double barrier,
                               double rebate, double rate, double vol, double time)
{
    if (spot >= barrier) return rebate * exp(-rate * time);
    if (barrier >= strike) {
        /* Standard case */
        double vanilla = mco_black_scholes_put(spot, strike, rate, vol, time);
        double in_price = mco_barrier_up_in_put(spot, strike, barrier, 0, rate, vol, time);
        return vanilla - in_price;
    }
    return 0.0;
}

double mco_barrier_up_in_put(double spot, double strike, double barrier,
                              double rebate, double rate, double vol, double time)
{
    (void) rebate;
    if (spot >= barrier) {
        return mco_black_scholes_put(spot, strike, rate, vol, time);
    }

    double sqrt_t = sqrt(time);
    double mu = (rate - 0.5 * vol * vol) / (vol * vol);
    double pow_term = pow(barrier / spot, 2.0 * mu);

    double y1 = log(barrier / spot) / (vol * sqrt_t) + (1.0 + mu) * vol * sqrt_t;

    return -spot * pow_term * norm_cdf(-y1) + strike * exp(-rate * time) * pow_term * norm_cdf(-y1 + vol * sqrt_t);
}

/*============================================================================
 * Public API
 *============================================================================*/

double mco_barrier_call(mco_ctx *ctx, double spot, double strike, double barrier,
                        double rebate, double rate, double vol, double time,
                        size_t steps, mco_barrier_style type)
{
    return mco_price_barrier(ctx, spot, strike, barrier, rebate, rate, vol, time,
                              steps, type, MCO_CALL);
}

double mco_barrier_put(mco_ctx *ctx, double spot, double strike, double barrier,
                       double rebate, double rate, double vol, double time,
                       size_t steps, mco_barrier_style type)
{
    return mco_price_barrier(ctx, spot, strike, barrier, rebate, rate, vol, time,
                              steps, type, MCO_PUT);
}
