/*
 * Heston Stochastic Volatility Model
 *
 * The Heston model (1993) is the most widely used stochastic volatility
 * model in equity derivatives. It models variance (not vol) as a
 * mean-reverting CIR process.
 *
 * Dynamics:
 *   dS = μ·S·dt + √v·S·dW₁
 *   dv = κ(θ - v)dt + σ·√v·dW₂
 *   dW₁·dW₂ = ρ·dt
 *
 * Where:
 *   S = Spot price
 *   v = Instantaneous variance (v = σ²)
 *   κ = Mean reversion speed (kappa)
 *   θ = Long-run variance (theta)
 *   σ = Vol of vol (sigma, often called ξ or η)
 *   ρ = Correlation (typically negative, -0.7 to -0.3)
 *
 * Parameters:
 *   v₀ (v0)    - Initial variance
 *   κ (kappa)  - Mean reversion speed (typical: 1-5)
 *   θ (theta)  - Long-run variance (typical: 0.04-0.09 for 20-30% vol)
 *   σ (sigma)  - Vol of vol (typical: 0.2-0.8)
 *   ρ (rho)    - Correlation (typical: -0.7 to -0.3)
 *
 * Feller condition (ensures v > 0):
 *   2κθ > σ²
 *
 * Key features:
 *   - Mean-reverting variance (vol clustering)
 *   - Negative correlation creates skew (leverage effect)
 *   - Semi-analytical pricing via characteristic function
 *   - Widely used for VIX derivatives
 *
 * Reference:
 *   Heston, S. (1993). "A Closed-Form Solution for Options with
 *   Stochastic Volatility with Applications to Bond and Currency Options"
 *   Review of Financial Studies, 6(2), 327-343
 */

#ifndef MCO_INTERNAL_MODELS_HESTON_H
#define MCO_INTERNAL_MODELS_HESTON_H

#include "internal/rng.h"
#include <stddef.h>
#include <math.h>

/*
 * Heston model parameters
 */
typedef struct {
    double spot;        /* Initial spot price S(0) */
    double v0;          /* Initial variance v(0) */
    double kappa;       /* Mean reversion speed κ */
    double theta;       /* Long-run variance θ */
    double sigma;       /* Vol of vol σ */
    double rho;         /* Correlation ρ */
    double rate;        /* Risk-free rate r */
    double time;        /* Time to maturity T */

    /* Precomputed */
    double discount;    /* e^(-rT) */
    double sqrt_rho;    /* √(1 - ρ²) for Cholesky */
} mco_heston;

/*
 * Heston path model for discrete simulation
 */
typedef struct {
    double spot;
    double v0;
    double kappa;
    double theta;
    double sigma;
    double rho;
    double rate;
    double dt;
    double sqrt_dt;
    double sqrt_rho;
    double discount;
    size_t num_steps;
} mco_heston_path;

/*
 * Initialize Heston model
 */
static inline void mco_heston_init(mco_heston *model,
                                    double spot,
                                    double v0,
                                    double kappa,
                                    double theta,
                                    double sigma,
                                    double rho,
                                    double rate,
                                    double time)
{
    model->spot     = spot;
    model->v0       = v0;
    model->kappa    = kappa;
    model->theta    = theta;
    model->sigma    = sigma;
    model->rho      = rho;
    model->rate     = rate;
    model->time     = time;
    model->discount = exp(-rate * time);
    model->sqrt_rho = sqrt(1.0 - rho * rho);
}

/*
 * Check Feller condition: 2κθ > σ²
 * Returns 1 if satisfied, 0 if not
 */
static inline int mco_heston_feller_ok(const mco_heston *model)
{
    return (2.0 * model->kappa * model->theta) > (model->sigma * model->sigma);
}

/*
 * Initialize Heston path model
 */
static inline void mco_heston_path_init(mco_heston_path *model,
                                         double spot,
                                         double v0,
                                         double kappa,
                                         double theta,
                                         double sigma,
                                         double rho,
                                         double rate,
                                         double time,
                                         size_t num_steps)
{
    model->spot      = spot;
    model->v0        = v0;
    model->kappa     = kappa;
    model->theta     = theta;
    model->sigma     = sigma;
    model->rho       = rho;
    model->rate      = rate;
    model->dt        = time / (double)num_steps;
    model->sqrt_dt   = sqrt(model->dt);
    model->sqrt_rho  = sqrt(1.0 - rho * rho);
    model->discount  = exp(-rate * time);
    model->num_steps = num_steps;
}

/*
 * Generate correlated Brownian increments
 */
static inline void mco_heston_correlated_normals(mco_rng *rng,
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
 * Euler-Maruyama step with full truncation scheme
 *
 * Full truncation: use max(v, 0) everywhere to keep variance positive
 * This is simple but can bias results slightly
 */
static inline void mco_heston_step_euler(const mco_heston_path *model,
                                          double *spot,
                                          double *var,
                                          double w1,
                                          double w2)
{
    double s = *spot;
    double v = *var;

    /* Truncate variance to ensure positivity */
    double v_plus = fmax(v, 0.0);
    double sqrt_v = sqrt(v_plus);

    /* Spot dynamics: dS = r·S·dt + √v·S·dW₁ */
    double ds = model->rate * s * model->dt + sqrt_v * s * model->sqrt_dt * w1;

    /* Variance dynamics: dv = κ(θ-v)dt + σ√v·dW₂ */
    double dv = model->kappa * (model->theta - v_plus) * model->dt
              + model->sigma * sqrt_v * model->sqrt_dt * w2;

    *spot = s + ds;
    *var  = v + dv;
}

/*
 * QE (Quadratic Exponential) scheme - more accurate for variance
 *
 * Andersen (2008) scheme that better preserves variance distribution
 */
static inline void mco_heston_step_qe(const mco_heston_path *model,
                                       double *spot,
                                       double *var,
                                       mco_rng *rng)
{
    double s = *spot;
    double v = *var;
    double dt = model->dt;

    /* Ensure positive variance */
    v = fmax(v, 0.0);

    /* QE scheme parameters */
    double exp_kdt = exp(-model->kappa * dt);
    double m = model->theta + (v - model->theta) * exp_kdt;
    double s2 = v * model->sigma * model->sigma * exp_kdt * (1.0 - exp_kdt) / model->kappa
              + model->theta * model->sigma * model->sigma * (1.0 - exp_kdt) * (1.0 - exp_kdt)
                / (2.0 * model->kappa);

    double psi = s2 / (m * m);
    double psi_crit = 1.5;

    double v_next;
    if (psi <= psi_crit) {
        /* Use quadratic approximation */
        double b2 = 2.0 / psi - 1.0 + sqrt(2.0 / psi) * sqrt(2.0 / psi - 1.0);
        double a = m / (1.0 + b2);
        double b = sqrt(b2);
        double z = mco_rng_normal(rng);
        v_next = a * (b + z) * (b + z);
    } else {
        /* Use exponential approximation */
        double p = (psi - 1.0) / (psi + 1.0);
        double beta = (1.0 - p) / m;
        double u = mco_rng_uniform(rng);
        if (u <= p) {
            v_next = 0.0;
        } else {
            v_next = log((1.0 - p) / (1.0 - u)) / beta;
        }
    }

    /* Spot update using integrated variance (approximation) */
    double v_avg = 0.5 * (v + v_next);
    double sqrt_v_avg = sqrt(fmax(v_avg, 0.0));
    double z1 = mco_rng_normal(rng);

    /* Log-spot dynamics */
    double log_s = log(s);
    double drift = (model->rate - 0.5 * v_avg) * dt;
    double diffusion = sqrt_v_avg * model->sqrt_dt * z1;
    /* Add correlation effect */
    diffusion += model->rho / model->sigma * (v_next - v - model->kappa * (model->theta - v) * dt);

    *spot = exp(log_s + drift + diffusion);
    *var  = v_next;
}

/*
 * Simulate Heston path using Euler scheme
 */
static inline double mco_heston_simulate_path(const mco_heston_path *model,
                                               mco_rng *rng,
                                               double *spot_path,
                                               double *var_path)
{
    double s = model->spot;
    double v = model->v0;

    if (spot_path) spot_path[0] = s;
    if (var_path) var_path[0] = v;

    for (size_t i = 0; i < model->num_steps; ++i) {
        double w1, w2;
        mco_heston_correlated_normals(rng, model->rho, model->sqrt_rho, &w1, &w2);
        mco_heston_step_euler(model, &s, &v, w1, w2);

        if (spot_path) spot_path[i + 1] = s;
        if (var_path) var_path[i + 1] = v;
    }

    return s;
}

/*
 * Simulate terminal spot only
 */
static inline double mco_heston_simulate_terminal(const mco_heston_path *model,
                                                   mco_rng *rng)
{
    return mco_heston_simulate_path(model, rng, NULL, NULL);
}

/*============================================================================
 * Heston Characteristic Function (for Fourier pricing)
 *============================================================================*/

/*
 * Heston characteristic function φ(u) = E[exp(iu·log(S(T)))]
 *
 * Used for FFT/COS method pricing (more accurate than MC for Europeans)
 *
 * This is a complex function - we return real and imaginary parts
 */
void mco_heston_char_func(double u,
                          double spot, double v0, double kappa, double theta,
                          double sigma, double rho, double rate, double time,
                          double *re, double *im);

#endif /* MCO_INTERNAL_MODELS_HESTON_H */
