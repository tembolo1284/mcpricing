/*
 * SABR Stochastic Volatility Model
 *
 * The SABR model (Hagan et al. 2002) captures volatility smile dynamics:
 *
 *   dF = σ · F^β · dW₁
 *   dσ = ν · σ · dW₂
 *   dW₁ · dW₂ = ρ · dt
 *
 * Where:
 *   F = Forward price
 *   σ = Stochastic volatility (starts at α)
 *   β = CEV exponent (0 = normal, 1 = lognormal)
 *   ρ = Correlation between F and σ (-1 < ρ < 1)
 *   ν = Vol of vol
 *
 * Parameters:
 *   α (alpha) - Initial volatility level
 *   β (beta)  - CEV exponent, typically 0 < β < 1
 *   ρ (rho)   - Correlation, typically -0.5 to 0.5
 *   ν (nu)    - Vol of vol, typically 0.2 to 0.8
 *
 * The model produces:
 *   - Volatility smile (OTM options have higher implied vol)
 *   - Skew (puts vs calls have different implied vols)
 *   - Term structure effects
 *
 * Reference:
 *   Hagan, P.S., Kumar, D., Lesniewski, A.S., Woodward, D.E. (2002)
 *   "Managing Smile Risk", Wilmott Magazine, September 2002
 */

#ifndef MCO_INTERNAL_MODELS_SABR_H
#define MCO_INTERNAL_MODELS_SABR_H

#include "internal/rng.h"
#include <stddef.h>
#include <math.h>

/*
 * SABR model parameters
 */
typedef struct {
    double forward;     /* Initial forward price F(0) */
    double alpha;       /* Initial volatility σ(0) */
    double beta;        /* CEV exponent (0 to 1) */
    double rho;         /* Correlation (-1 to 1) */
    double nu;          /* Vol of vol */
    double time;        /* Time to maturity */
    double rate;        /* Risk-free rate (for discounting) */

    /* Precomputed */
    double discount;    /* exp(-r·T) */
    double sqrt_rho;    /* sqrt(1 - ρ²) for Cholesky */
} mco_sabr;

/*
 * SABR path simulation parameters (discrete steps)
 */
typedef struct {
    double forward;
    double alpha;
    double beta;
    double rho;
    double nu;
    double dt;          /* Time step */
    double sqrt_dt;     /* sqrt(dt) */
    double sqrt_rho;    /* sqrt(1 - ρ²) */
    double discount;
    size_t num_steps;
} mco_sabr_path;

/*
 * Initialize SABR model
 */
static inline void mco_sabr_init(mco_sabr *model,
                                  double forward,
                                  double alpha,
                                  double beta,
                                  double rho,
                                  double nu,
                                  double time,
                                  double rate)
{
    model->forward  = forward;
    model->alpha    = alpha;
    model->beta     = beta;
    model->rho      = rho;
    model->nu       = nu;
    model->time     = time;
    model->rate     = rate;
    model->discount = exp(-rate * time);
    model->sqrt_rho = sqrt(1.0 - rho * rho);
}

/*
 * Initialize SABR path model for discrete simulation
 */
static inline void mco_sabr_path_init(mco_sabr_path *model,
                                       double forward,
                                       double alpha,
                                       double beta,
                                       double rho,
                                       double nu,
                                       double time,
                                       double rate,
                                       size_t num_steps)
{
    model->forward   = forward;
    model->alpha     = alpha;
    model->beta      = beta;
    model->rho       = rho;
    model->nu        = nu;
    model->dt        = time / (double)num_steps;
    model->sqrt_dt   = sqrt(model->dt);
    model->sqrt_rho  = sqrt(1.0 - rho * rho);
    model->discount  = exp(-rate * time);
    model->num_steps = num_steps;
}

/*
 * Generate correlated Brownian increments using Cholesky decomposition.
 *
 * Given independent Z1, Z2 ~ N(0,1):
 *   W1 = Z1
 *   W2 = ρ·Z1 + sqrt(1-ρ²)·Z2
 *
 * Then Corr(W1, W2) = ρ
 */
static inline void mco_sabr_correlated_normals(mco_rng *rng,
                                                double rho,
                                                double sqrt_rho,
                                                double *w1,
                                                double *w2)
{
    double z1 = mco_rng_normal(rng);
    double z2 = mco_rng_normal(rng);

    *w1 = z1;
    *w2 = rho * z1 + sqrt_rho * z2;
}

/*
 * Simulate one step of SABR dynamics (Euler-Maruyama scheme)
 *
 * F(t+dt) = F(t) + σ(t)·F(t)^β·√dt·W1
 * σ(t+dt) = σ(t) + ν·σ(t)·√dt·W2
 *
 * With absorption at F = 0 and σ = 0
 */
static inline void mco_sabr_step(const mco_sabr_path *model,
                                  double *forward,
                                  double *sigma,
                                  double w1,
                                  double w2)
{
    double f = *forward;
    double s = *sigma;

    /* Absorption boundaries */
    if (f <= 0.0) {
        *forward = 0.0;
        return;
    }
    if (s <= 0.0) {
        s = 1e-10;  /* Small positive to avoid division by zero */
    }

    /* F^β term */
    double f_beta = pow(f, model->beta);

    /* Euler step */
    double df = s * f_beta * model->sqrt_dt * w1;
    double ds = model->nu * s * model->sqrt_dt * w2;

    *forward = fmax(f + df, 0.0);  /* Absorb at 0 */
    *sigma   = fmax(s + ds, 0.0);
}

/*
 * Simulate full SABR path
 *
 * Returns terminal forward price
 */
static inline double mco_sabr_simulate_path(const mco_sabr_path *model,
                                             mco_rng *rng,
                                             double *path)  /* Optional: can be NULL */
{
    double f = model->forward;
    double s = model->alpha;

    if (path) path[0] = f;

    for (size_t i = 0; i < model->num_steps; ++i) {
        double w1, w2;
        mco_sabr_correlated_normals(rng, model->rho, model->sqrt_rho, &w1, &w2);
        mco_sabr_step(model, &f, &s, w1, w2);

        if (path) path[i + 1] = f;
    }

    return f;
}

/*
 * Simulate terminal forward price only (no path storage)
 */
static inline double mco_sabr_simulate_terminal(const mco_sabr_path *model,
                                                 mco_rng *rng)
{
    return mco_sabr_simulate_path(model, rng, NULL);
}

/*============================================================================
 * Hagan SABR Implied Volatility Approximation
 *============================================================================*/

/*
 * Hagan et al. (2002) closed-form approximation for SABR implied volatility.
 *
 * This is the standard formula used in practice for calibration.
 * Valid for β ∈ (0, 1), less accurate for very short expiries or
 * extreme strikes.
 *
 * Parameters:
 *   forward - Forward price F
 *   strike  - Strike price K
 *   time    - Time to expiry T
 *   alpha   - SABR α
 *   beta    - SABR β
 *   rho     - SABR ρ
 *   nu      - SABR ν
 *
 * Returns:
 *   Black implied volatility σ_B
 */
double mco_sabr_implied_vol(double forward,
                            double strike,
                            double time,
                            double alpha,
                            double beta,
                            double rho,
                            double nu);

/*
 * SABR implied vol for ATM (F = K) - simplified formula
 */
double mco_sabr_atm_vol(double forward,
                        double time,
                        double alpha,
                        double beta,
                        double rho,
                        double nu);

#endif /* MCO_INTERNAL_MODELS_SABR_H */
