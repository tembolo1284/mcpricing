/*
 * Merton Jump-Diffusion Model
 *
 * The Merton model (1976) adds Poisson jumps to GBM, capturing sudden
 * price movements from earnings, news, or market crashes.
 *
 * Dynamics:
 *   dS/S = (μ - λk)dt + σdW + (J-1)dN
 *
 * Where:
 *   σ = Diffusion volatility
 *   λ = Jump intensity (expected jumps per year)
 *   N = Poisson process with intensity λ
 *   J = Jump size multiplier (J-1 is the percentage jump)
 *   k = E[J-1] = exp(μⱼ + σⱼ²/2) - 1 (compensator)
 *
 * Jump distribution:
 *   log(J) ~ N(μⱼ, σⱼ²)
 *   J = exp(μⱼ + σⱼ·Z) where Z ~ N(0,1)
 *
 * Parameters:
 *   σ (sigma)    - Diffusion volatility (typical: 0.15-0.25)
 *   λ (lambda)   - Jump intensity, jumps/year (typical: 0.5-3)
 *   μⱼ (mu_j)    - Mean log-jump size (typical: -0.1 for crashes)
 *   σⱼ (sigma_j) - Volatility of log-jump (typical: 0.1-0.3)
 *
 * Key features:
 *   - Captures fat tails (kurtosis > 3)
 *   - Models earnings surprises, flash crashes
 *   - Short-dated smile steeper than long-dated
 *   - Has closed-form solution (infinite series)
 *
 * Use cases:
 *   - Short-dated options (weeklies)
 *   - Options around earnings announcements
 *   - Crash modeling, tail risk
 *
 * Reference:
 *   Merton, R.C. (1976). "Option Pricing when Underlying Stock Returns
 *   are Discontinuous". Journal of Financial Economics, 3, 125-144
 */

#ifndef MCO_INTERNAL_MODELS_MERTON_JUMP_H
#define MCO_INTERNAL_MODELS_MERTON_JUMP_H

#include "internal/rng.h"
#include <stddef.h>
#include <math.h>

/*
 * Merton Jump-Diffusion parameters
 */
typedef struct {
    double spot;        /* Initial spot price S(0) */
    double rate;        /* Risk-free rate r */
    double sigma;       /* Diffusion volatility σ */
    double lambda;      /* Jump intensity λ (jumps/year) */
    double mu_j;        /* Mean log-jump μⱼ */
    double sigma_j;     /* Vol of log-jump σⱼ */
    double time;        /* Time to maturity T */

    /* Precomputed */
    double k;           /* Compensator k = E[J-1] */
    double discount;    /* e^(-rT) */
} mco_merton;

/*
 * Merton path model for discrete simulation
 */
typedef struct {
    double spot;
    double rate;
    double sigma;
    double lambda;
    double mu_j;
    double sigma_j;
    double dt;
    double sqrt_dt;
    double k;
    double discount;
    size_t num_steps;
} mco_merton_path;

/*
 * Initialize Merton model
 */
static inline void mco_merton_init(mco_merton *model,
                                    double spot,
                                    double rate,
                                    double sigma,
                                    double lambda,
                                    double mu_j,
                                    double sigma_j,
                                    double time)
{
    model->spot     = spot;
    model->rate     = rate;
    model->sigma    = sigma;
    model->lambda   = lambda;
    model->mu_j     = mu_j;
    model->sigma_j  = sigma_j;
    model->time     = time;

    /* k = E[J-1] = exp(μⱼ + σⱼ²/2) - 1 */
    model->k = exp(mu_j + 0.5 * sigma_j * sigma_j) - 1.0;
    model->discount = exp(-rate * time);
}

/*
 * Initialize Merton path model
 */
static inline void mco_merton_path_init(mco_merton_path *model,
                                         double spot,
                                         double rate,
                                         double sigma,
                                         double lambda,
                                         double mu_j,
                                         double sigma_j,
                                         double time,
                                         size_t num_steps)
{
    model->spot      = spot;
    model->rate      = rate;
    model->sigma     = sigma;
    model->lambda    = lambda;
    model->mu_j      = mu_j;
    model->sigma_j   = sigma_j;
    model->dt        = time / (double)num_steps;
    model->sqrt_dt   = sqrt(model->dt);
    model->k         = exp(mu_j + 0.5 * sigma_j * sigma_j) - 1.0;
    model->discount  = exp(-rate * time);
    model->num_steps = num_steps;
}

/*
 * Generate Poisson random variable using inverse transform
 *
 * For λdt small, we can use Bernoulli approximation
 */
static inline int mco_poisson(mco_rng *rng, double lambda_dt)
{
    if (lambda_dt <= 0.0) return 0;

    /* For small λdt, approximate with Bernoulli */
    if (lambda_dt < 0.1) {
        return (mco_rng_uniform(rng) < lambda_dt) ? 1 : 0;
    }

    /* Inverse transform method */
    double L = exp(-lambda_dt);
    double p = 1.0;
    int k = 0;

    do {
        k++;
        p *= mco_rng_uniform(rng);
    } while (p > L);

    return k - 1;
}

/*
 * Simulate one step of Merton dynamics
 *
 * Using Euler discretization:
 *   S(t+dt) = S(t) · exp((r - λk - σ²/2)dt + σ√dt·Z + Σⱼ log(Jⱼ))
 *
 * Where the sum is over N ~ Poisson(λdt) jumps
 */
static inline void mco_merton_step(const mco_merton_path *model,
                                    double *spot,
                                    mco_rng *rng)
{
    double s = *spot;
    double z = mco_rng_normal(rng);

    /* Diffusion part */
    double drift = (model->rate - model->lambda * model->k
                    - 0.5 * model->sigma * model->sigma) * model->dt;
    double diffusion = model->sigma * model->sqrt_dt * z;

    /* Jump part */
    int num_jumps = mco_poisson(rng, model->lambda * model->dt);
    double jump_sum = 0.0;

    for (int j = 0; j < num_jumps; ++j) {
        double zj = mco_rng_normal(rng);
        jump_sum += model->mu_j + model->sigma_j * zj;
    }

    *spot = s * exp(drift + diffusion + jump_sum);
}

/*
 * Simulate Merton path
 */
static inline double mco_merton_simulate_path(const mco_merton_path *model,
                                               mco_rng *rng,
                                               double *path)
{
    double s = model->spot;

    if (path) path[0] = s;

    for (size_t i = 0; i < model->num_steps; ++i) {
        mco_merton_step(model, &s, rng);
        if (path) path[i + 1] = s;
    }

    return s;
}

/*
 * Simulate terminal spot only
 */
static inline double mco_merton_simulate_terminal(const mco_merton_path *model,
                                                   mco_rng *rng)
{
    return mco_merton_simulate_path(model, rng, NULL);
}

/*============================================================================
 * Merton Analytical Formula (Infinite Series)
 *============================================================================*/

/*
 * Merton call price (series expansion)
 *
 * The Merton formula expresses the option price as a weighted sum of
 * Black-Scholes prices with adjusted parameters:
 *
 *   C = Σₙ [e^(-λ'T)(λ'T)ⁿ/n!] · BS(S, K, rₙ, σₙ, T)
 *
 * Where:
 *   λ' = λ(1 + k)
 *   rₙ = r - λk + n·log(1+k)/T
 *   σₙ² = σ² + n·σⱼ²/T
 *
 * In practice, we sum ~50 terms for convergence.
 */
double mco_merton_call(double spot, double strike, double rate, double time,
                       double sigma, double lambda, double mu_j, double sigma_j);

double mco_merton_put(double spot, double strike, double rate, double time,
                      double sigma, double lambda, double mu_j, double sigma_j);

#endif /* MCO_INTERNAL_MODELS_MERTON_JUMP_H */
