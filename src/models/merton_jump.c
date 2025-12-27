/*
 * Merton Jump-Diffusion Implementation
 *
 * Monte Carlo simulation and closed-form series solution.
 */

#include "internal/models/merton_jump.h"
#include "internal/instruments/payoff.h"
#include "internal/context.h"
#include "mcoptions.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * Factorial lookup for small n
 */
static double factorial(int n)
{
    static const double fact[] = {
        1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800,
        39916800, 479001600, 6227020800.0, 87178291200.0, 1307674368000.0
    };
    if (n < 0) return 1;
    if (n < 16) return fact[n];

    double f = fact[15];
    for (int i = 16; i <= n; ++i) {
        f *= i;
    }
    return f;
}

/*============================================================================
 * Black-Scholes Helper (for Merton series)
 *============================================================================*/

static double norm_cdf(double x)
{
    return 0.5 * erfc(-x * 0.7071067811865475);
}

static double bs_call_internal(double spot, double strike, double rate,
                               double vol, double time)
{
    if (time <= 0.0) return fmax(spot - strike, 0.0);
    if (vol <= 0.0) return fmax(spot - strike * exp(-rate * time), 0.0);

    double sqrt_t = sqrt(time);
    double d1 = (log(spot / strike) + (rate + 0.5 * vol * vol) * time) / (vol * sqrt_t);
    double d2 = d1 - vol * sqrt_t;

    return spot * norm_cdf(d1) - strike * exp(-rate * time) * norm_cdf(d2);
}

/*============================================================================
 * Merton Analytical Formula
 *============================================================================*/

double mco_merton_call(double spot, double strike, double rate, double time,
                       double sigma, double lambda, double mu_j, double sigma_j)
{
    if (spot <= 0.0 || strike <= 0.0 || time <= 0.0) {
        return fmax(spot - strike, 0.0);
    }

    /* k = E[J-1] = exp(μⱼ + σⱼ²/2) - 1 */
    double k = exp(mu_j + 0.5 * sigma_j * sigma_j) - 1.0;

    /* λ' = λ(1 + k) */
    double lambda_prime = lambda * (1.0 + k);

    /* Sum over Poisson terms */
    double price = 0.0;
    double poisson_weight = exp(-lambda_prime * time);

    /* Sum ~50 terms (usually converges much faster) */
    for (int n = 0; n < 50; ++n) {
        if (n > 0) {
            poisson_weight *= (lambda_prime * time) / n;
        }

        /* Adjusted parameters for n jumps */
        double r_n = rate - lambda * k + n * log(1.0 + k) / time;
        double sigma_n_sq = sigma * sigma + n * sigma_j * sigma_j / time;
        double sigma_n = sqrt(sigma_n_sq);

        /* BS price with adjusted parameters */
        double bs_n = bs_call_internal(spot, strike, r_n, sigma_n, time);

        price += poisson_weight * bs_n;

        /* Check for convergence */
        if (poisson_weight < 1e-15 && n > 10) break;
    }

    return price;
}

double mco_merton_put(double spot, double strike, double rate, double time,
                      double sigma, double lambda, double mu_j, double sigma_j)
{
    /* Use put-call parity */
    double call = mco_merton_call(spot, strike, rate, time, sigma, lambda, mu_j, sigma_j);
    return call - spot + strike * exp(-rate * time);
}

/*============================================================================
 * Monte Carlo Pricing
 *============================================================================*/

static double price_merton_european(mco_ctx *ctx,
                                    double spot,
                                    double strike,
                                    double rate,
                                    double time,
                                    double sigma,
                                    double lambda,
                                    double mu_j,
                                    double sigma_j,
                                    mco_option_type type)
{
    if (!ctx) return 0.0;

    /* Use more steps for jump process */
    size_t num_steps = ctx->num_steps;
    if (num_steps < 252) num_steps = 252;  /* Daily for jumps */

    uint64_t n_paths = ctx->num_simulations;

    /* Initialize Merton path model */
    mco_merton_path model;
    mco_merton_path_init(&model, spot, rate, sigma, lambda, mu_j, sigma_j,
                          time, num_steps);

    double sum_payoff = 0.0;
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        double s_T = mco_merton_simulate_terminal(&model, &rng);
        double payoff = mco_payoff(s_T, strike, type);
        sum_payoff += payoff;
    }

    double price = model.discount * (sum_payoff / (double)n_paths);
    return price;
}

/*============================================================================
 * Public API
 *============================================================================*/

double mco_merton_european_call(mco_ctx *ctx,
                                double spot,
                                double strike,
                                double rate,
                                double time,
                                double sigma,
                                double lambda,
                                double mu_j,
                                double sigma_j)
{
    return price_merton_european(ctx, spot, strike, rate, time,
                                  sigma, lambda, mu_j, sigma_j, MCO_CALL);
}

double mco_merton_european_put(mco_ctx *ctx,
                               double spot,
                               double strike,
                               double rate,
                               double time,
                               double sigma,
                               double lambda,
                               double mu_j,
                               double sigma_j)
{
    return price_merton_european(ctx, spot, strike, rate, time,
                                  sigma, lambda, mu_j, sigma_j, MCO_PUT);
}
