/*
 * Heston Model Implementation
 *
 * Monte Carlo simulation and characteristic function for Fourier pricing.
 */

#include "internal/models/heston.h"
#include "internal/instruments/payoff.h"
#include "internal/context.h"
#include "mcoptions.h"
#include <math.h>
#include <complex.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*============================================================================
 * Characteristic Function
 *============================================================================*/

/*
 * Heston characteristic function (Gatheral formulation)
 *
 * φ(u) = exp(C + D·v₀ + iu·log(S₀·e^(rT)))
 *
 * Where C and D are complex functions of the parameters.
 */
void mco_heston_char_func(double u,
                          double spot, double v0, double kappa, double theta,
                          double sigma, double rho, double rate, double time,
                          double *re, double *im)
{
    /* Use complex arithmetic */
    double complex iu = (double complex)I * u;

    /* d = √((ρσiu - κ)² + σ²(iu + u²)) */
    double complex a = rho * sigma * iu - kappa;
    double complex b = sigma * sigma * (iu + u * u);
    double complex d = csqrt(a * a + b);

    /* g = (κ - ρσiu - d) / (κ - ρσiu + d) */
    double complex g_num = kappa - rho * sigma * iu - d;
    double complex g_den = kappa - rho * sigma * iu + d;
    double complex g = g_num / g_den;

    /* exp(-dT) */
    double complex exp_dT = cexp(-d * time);

    /* C = rT·iu + (κθ/σ²)·[(κ - ρσiu - d)T - 2log((1 - g·e^(-dT))/(1-g))] */
    double complex C_term1 = rate * time * iu;
    double complex C_term2_inner = (1.0 - g * exp_dT) / (1.0 - g);
    double complex C_term2 = (kappa * theta / (sigma * sigma))
                           * (g_num * time - 2.0 * clog(C_term2_inner));
    double complex C = C_term1 + C_term2;

    /* D = ((κ - ρσiu - d)/σ²)·((1 - e^(-dT))/(1 - g·e^(-dT))) */
    double complex D = (g_num / (sigma * sigma))
                     * ((1.0 - exp_dT) / (1.0 - g * exp_dT));

    /* φ = exp(C + D·v₀ + iu·log(S₀)) */
    double complex phi = cexp(C + D * v0 + iu * log(spot));

    *re = creal(phi);
    *im = cimag(phi);
}

/*============================================================================
 * Monte Carlo Pricing
 *============================================================================*/

static double price_heston_european(mco_ctx *ctx,
                                    double spot,
                                    double strike,
                                    double rate,
                                    double time,
                                    double v0,
                                    double kappa,
                                    double theta,
                                    double sigma,
                                    double rho,
                                    mco_option_type type)
{
    if (!ctx) return 0.0;

    /* Use more steps for stochastic vol */
    size_t num_steps = ctx->num_steps;
    if (num_steps < 100) num_steps = 100;

    uint64_t n_paths = ctx->num_simulations;

    /* Initialize Heston path model */
    mco_heston_path model;
    mco_heston_path_init(&model, spot, v0, kappa, theta, sigma, rho,
                          rate, time, num_steps);

    double sum_payoff = 0.0;
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        double s_T = mco_heston_simulate_terminal(&model, &rng);
        double payoff = mco_payoff(s_T, strike, type);
        sum_payoff += payoff;
    }

    double price = model.discount * (sum_payoff / (double)n_paths);
    return price;
}

/*============================================================================
 * Public API
 *============================================================================*/

double mco_heston_european_call(mco_ctx *ctx,
                                double spot,
                                double strike,
                                double rate,
                                double time,
                                double v0,
                                double kappa,
                                double theta,
                                double sigma,
                                double rho)
{
    return price_heston_european(ctx, spot, strike, rate, time,
                                  v0, kappa, theta, sigma, rho, MCO_CALL);
}

double mco_heston_european_put(mco_ctx *ctx,
                               double spot,
                               double strike,
                               double rate,
                               double time,
                               double v0,
                               double kappa,
                               double theta,
                               double sigma,
                               double rho)
{
    return price_heston_european(ctx, spot, strike, rate, time,
                                  v0, kappa, theta, sigma, rho, MCO_PUT);
}

/*
 * Check if Feller condition is satisfied
 */
int mco_heston_check_feller(double kappa, double theta, double sigma)
{
    return (2.0 * kappa * theta) > (sigma * sigma);
}
