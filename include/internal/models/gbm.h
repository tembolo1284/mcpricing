/*
 * Geometric Brownian Motion (GBM) Model
 *
 * The standard model for equity price dynamics:
 *
 *   dS = μ·S·dt + σ·S·dW
 *
 * Where:
 *   S  = spot price
 *   μ  = drift (under risk-neutral measure: μ = r)
 *   σ  = volatility
 *   dW = Wiener process increment
 *
 * Exact solution (used for simulation):
 *
 *   S(t) = S(0) · exp((r - ½σ²)t + σ√t·Z)
 *
 * Where Z ~ N(0,1)
 *
 * Properties:
 *   - Log-normal distribution of prices
 *   - Constant volatility (unrealistic but tractable)
 *   - No jumps, no stochastic volatility
 *   - Closed-form European prices via Black-Scholes
 */

#ifndef MCO_INTERNAL_MODELS_GBM_H
#define MCO_INTERNAL_MODELS_GBM_H

#include "internal/rng.h"
#include <stddef.h>
#include <math.h>

/*
 * GBM model parameters
 *
 * Precomputed for efficiency when running many simulations.
 */
typedef struct {
    double spot;           /* Initial spot price S(0) */
    double rate;           /* Risk-free rate r */
    double volatility;     /* Volatility σ */
    double time;           /* Time to maturity T */

    /* Precomputed constants */
    double drift;          /* (r - 0.5·σ²)·T */
    double diffusion;      /* σ·√T */
    double discount;       /* exp(-r·T) */
} mco_gbm;

/*
 * Initialize GBM model with precomputed constants.
 *
 * Call this once before running simulations.
 */
static inline void mco_gbm_init(mco_gbm *model,
                                double spot,
                                double rate,
                                double volatility,
                                double time)
{
    model->spot       = spot;
    model->rate       = rate;
    model->volatility = volatility;
    model->time       = time;

    model->drift      = (rate - 0.5 * volatility * volatility) * time;
    model->diffusion  = volatility * sqrt(time);
    model->discount   = exp(-rate * time);
}

/*
 * Simulate terminal spot price S(T) given a standard normal Z.
 *
 * This is the core simulation step - everything else builds on this.
 */
static inline double mco_gbm_terminal(const mco_gbm *model, double z)
{
    return model->spot * exp(model->drift + model->diffusion * z);
}

/*
 * Simulate terminal spot price using internal RNG.
 */
static inline double mco_gbm_simulate(const mco_gbm *model, mco_rng *rng)
{
    double z = mco_rng_normal(rng);
    return mco_gbm_terminal(model, z);
}

/*
 * Simulate terminal spot price with antithetic variate.
 *
 * Returns S(T) using +Z, writes S(T) using -Z to *antithetic_out.
 * Both paths use the same random draw, reducing variance.
 */
static inline double mco_gbm_simulate_antithetic(const mco_gbm *model,
                                                  mco_rng *rng,
                                                  double *antithetic_out)
{
    double z = mco_rng_normal(rng);
    *antithetic_out = mco_gbm_terminal(model, -z);
    return mco_gbm_terminal(model, z);
}

/*
 * GBM parameters for path simulation (discrete steps)
 *
 * Used for path-dependent options: Asian, Barrier, Lookback
 */
typedef struct {
    double spot;           /* Initial spot price S(0) */
    double dt;             /* Time step size: T / num_steps */
    double drift_dt;       /* (r - 0.5·σ²)·dt */
    double diffusion_dt;   /* σ·√dt */
    double discount;       /* exp(-r·T) */
    size_t num_steps;      /* Number of time steps */
} mco_gbm_path;

/*
 * Initialize GBM path model with precomputed step constants.
 */
static inline void mco_gbm_path_init(mco_gbm_path *model,
                                     double spot,
                                     double rate,
                                     double volatility,
                                     double time,
                                     size_t num_steps)
{
    double dt = time / (double)num_steps;

    model->spot         = spot;
    model->dt           = dt;
    model->drift_dt     = (rate - 0.5 * volatility * volatility) * dt;
    model->diffusion_dt = volatility * sqrt(dt);
    model->discount     = exp(-rate * time);
    model->num_steps    = num_steps;
}

/*
 * Advance spot price by one time step.
 *
 * S(t+dt) = S(t) · exp(drift_dt + diffusion_dt · Z)
 */
static inline double mco_gbm_step(const mco_gbm_path *model,
                                  double current_spot,
                                  double z)
{
    return current_spot * exp(model->drift_dt + model->diffusion_dt * z);
}

/*
 * Simulate a full path, storing all intermediate prices.
 *
 * Parameters:
 *   model - Initialized path model
 *   rng   - Thread-local RNG
 *   path  - Output array of size (num_steps + 1)
 *
 * After call:
 *   path[0] = spot
 *   path[i] = S(i·dt) for i = 1..num_steps
 */
static inline void mco_gbm_simulate_path(const mco_gbm_path *model,
                                         mco_rng *rng,
                                         double *path)
{
    path[0] = model->spot;

    for (size_t i = 0; i < model->num_steps; ++i) {
        double z = mco_rng_normal(rng);
        path[i + 1] = mco_gbm_step(model, path[i], z);
    }
}

/*
 * Black-Scholes closed-form solution (for validation & control variates)
 *
 * N(x) = CDF of standard normal distribution
 *
 * Call: S·N(d1) - K·e^(-rT)·N(d2)
 * Put:  K·e^(-rT)·N(-d2) - S·N(-d1)
 *
 * Where:
 *   d1 = (ln(S/K) + (r + σ²/2)T) / (σ√T)
 *   d2 = d1 - σ√T
 */
double mco_black_scholes_call(double spot, double strike, double rate,
                              double volatility, double time);

double mco_black_scholes_put(double spot, double strike, double rate,
                             double volatility, double time);

#endif /* MCO_INTERNAL_MODELS_GBM_H */
