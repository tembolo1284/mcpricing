/*
 * Black-76 Model for Futures/Forward Options
 *
 * The Black-76 model (Fischer Black, 1976) prices options on futures
 * and forward contracts. Unlike Black-Scholes which uses spot price,
 * Black-76 uses the forward/futures price directly.
 *
 * Key differences from Black-Scholes:
 *   - No cost of carry (forward already incorporates it)
 *   - Discount the entire payoff, not just the strike
 *   - Used for: commodity options, interest rate caps/floors, swaptions
 *
 * Formula:
 *   Call = e^(-rT) · [F·N(d₁) - K·N(d₂)]
 *   Put  = e^(-rT) · [K·N(-d₂) - F·N(-d₁)]
 *
 * Where:
 *   d₁ = [ln(F/K) + σ²T/2] / (σ√T)
 *   d₂ = d₁ - σ√T
 *   F = Forward/futures price
 *   K = Strike
 *   σ = Volatility
 *   T = Time to expiry
 *   r = Risk-free rate (for discounting)
 *
 * Reference:
 *   Black, F. (1976). "The Pricing of Commodity Contracts"
 *   Journal of Financial Economics, 3, 167-179
 */

#ifndef MCO_INTERNAL_MODELS_BLACK76_H
#define MCO_INTERNAL_MODELS_BLACK76_H

#include "internal/rng.h"
#include <stddef.h>
#include <math.h>

/*
 * Black-76 model parameters for Monte Carlo
 */
typedef struct {
    double forward;      /* Forward/futures price F(0) */
    double volatility;   /* Volatility σ */
    double rate;         /* Risk-free rate (for discounting) */
    double time;         /* Time to maturity T */

    /* Precomputed constants */
    double drift;        /* -0.5·σ²·T (no cost of carry term) */
    double diffusion;    /* σ·√T */
    double discount;     /* e^(-rT) */
} mco_black76;

/*
 * Black-76 path model for discrete simulation
 */
typedef struct {
    double forward;
    double volatility;
    double rate;
    double dt;
    double sqrt_dt;
    double drift_dt;     /* -0.5·σ²·dt */
    double vol_sqrt_dt;  /* σ·√dt */
    double discount;
    size_t num_steps;
} mco_black76_path;

/*
 * Initialize Black-76 terminal model
 */
static inline void mco_black76_init(mco_black76 *model,
                                     double forward,
                                     double volatility,
                                     double rate,
                                     double time)
{
    model->forward    = forward;
    model->volatility = volatility;
    model->rate       = rate;
    model->time       = time;

    double sigma_sq = volatility * volatility;
    model->drift     = -0.5 * sigma_sq * time;
    model->diffusion = volatility * sqrt(time);
    model->discount  = exp(-rate * time);
}

/*
 * Initialize Black-76 path model
 */
static inline void mco_black76_path_init(mco_black76_path *model,
                                          double forward,
                                          double volatility,
                                          double rate,
                                          double time,
                                          size_t num_steps)
{
    model->forward    = forward;
    model->volatility = volatility;
    model->rate       = rate;
    model->dt         = time / (double)num_steps;
    model->sqrt_dt    = sqrt(model->dt);
    model->drift_dt   = -0.5 * volatility * volatility * model->dt;
    model->vol_sqrt_dt = volatility * model->sqrt_dt;
    model->discount   = exp(-rate * time);
    model->num_steps  = num_steps;
}

/*
 * Simulate terminal forward price (single step)
 *
 * F(T) = F(0) · exp(-σ²T/2 + σ√T·Z)
 */
static inline double mco_black76_simulate(const mco_black76 *model,
                                           mco_rng *rng)
{
    double z = mco_rng_normal(rng);
    return model->forward * exp(model->drift + model->diffusion * z);
}

/*
 * Simulate forward price path
 */
static inline double mco_black76_simulate_path(const mco_black76_path *model,
                                                mco_rng *rng,
                                                double *path)
{
    double f = model->forward;

    if (path) path[0] = f;

    for (size_t i = 0; i < model->num_steps; ++i) {
        double z = mco_rng_normal(rng);
        f *= exp(model->drift_dt + model->vol_sqrt_dt * z);
        if (path) path[i + 1] = f;
    }

    return f;
}

/*============================================================================
 * Black-76 Analytical Formulas
 *============================================================================*/

/*
 * Black-76 call price (analytical)
 */
double mco_black76_call(double forward,
                        double strike,
                        double rate,
                        double volatility,
                        double time);

/*
 * Black-76 put price (analytical)
 */
double mco_black76_put(double forward,
                       double strike,
                       double rate,
                       double volatility,
                       double time);

/*
 * Black-76 implied volatility (Newton-Raphson)
 */
double mco_black76_implied_vol(double forward,
                               double strike,
                               double rate,
                               double time,
                               double price,
                               int is_call);

/*
 * Black-76 Greeks
 */
double mco_black76_delta(double forward, double strike, double rate,
                         double volatility, double time, int is_call);

double mco_black76_gamma(double forward, double strike, double rate,
                         double volatility, double time);

double mco_black76_vega(double forward, double strike, double rate,
                        double volatility, double time);

double mco_black76_theta(double forward, double strike, double rate,
                         double volatility, double time, int is_call);

#endif /* MCO_INTERNAL_MODELS_BLACK76_H */
