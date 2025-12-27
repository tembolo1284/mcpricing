/*
 * SABR Model Implementation
 *
 * Hagan et al. (2002) implied Black volatility approximation.
 *
 * Reference: "Managing Smile Risk", Wilmott Magazine, Sept 2002
 *
 * The formula returns BLACK (lognormal) implied volatility that can
 * be used directly with the Black-Scholes formula.
 */

#include "internal/models/sabr.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*============================================================================
 * Hagan SABR Implied Volatility Formula
 *============================================================================*/

double mco_sabr_implied_vol(double forward,
                            double strike,
                            double time,
                            double alpha,
                            double beta,
                            double rho,
                            double nu)
{
    /* Validate inputs */
    if (alpha < 1e-10 || time < 1e-10 || forward <= 0.0 || strike <= 0.0) {
        return alpha;  /* Return alpha as fallback */
    }

    double F = forward;
    double K = strike;
    double T = time;

    /* Handle ATM case (F ≈ K) */
    if (fabs(F - K) < 1e-7 * F) {
        return mco_sabr_atm_vol(F, T, alpha, beta, rho, nu);
    }

    double one_beta = 1.0 - beta;
    double FK = F * K;
    double logFK = log(F / K);
    double logFK2 = logFK * logFK;
    double logFK4 = logFK2 * logFK2;

    /* (FK)^((1-β)/2) */
    double FK_mid = pow(FK, 0.5 * one_beta);

    /* (FK)^(1-β) */
    double FK_full = pow(FK, one_beta);

    /*
     * z = (ν/α) · (FK)^((1-β)/2) · log(F/K)
     */
    double z = (nu / alpha) * FK_mid * logFK;

    /*
     * χ(z) = log[(√(1 - 2ρz + z²) + z - ρ) / (1 - ρ)]
     *
     * For small z, χ(z) ≈ z, so z/χ(z) ≈ 1
     */
    double chi_z;
    if (fabs(z) < 1e-6) {
        chi_z = 1.0;  /* z/χ(z) → 1 as z → 0 */
    } else {
        double sqrt_term = sqrt(1.0 - 2.0 * rho * z + z * z);
        if (fabs(1.0 - rho) < 1e-10) {
            /* Limit as ρ → 1 */
            chi_z = z / (1.0 - 0.5 * z);
        } else {
            double x = log((sqrt_term + z - rho) / (1.0 - rho));
            chi_z = z / x;
        }
    }

    /*
     * Denominator: 1 + [(1-β)²/24]·log²(F/K) + [(1-β)⁴/1920]·log⁴(F/K)
     */
    double denom = 1.0 
        + (one_beta * one_beta / 24.0) * logFK2 
        + (one_beta * one_beta * one_beta * one_beta / 1920.0) * logFK4;

    /*
     * Numerator coefficient: α / [(FK)^((1-β)/2) · denom]
     */
    double num_coeff = alpha / (FK_mid * denom);

    /*
     * Time correction: 1 + εT where
     * ε = [(1-β)²/24]·[α²/(FK)^(1-β)] + [ρβνα/4]·[1/(FK)^((1-β)/2)] + [(2-3ρ²)/24]·ν²
     */
    double eps1 = (one_beta * one_beta / 24.0) * (alpha * alpha / FK_full);
    double eps2 = (rho * beta * nu * alpha / 4.0) / FK_mid;
    double eps3 = ((2.0 - 3.0 * rho * rho) / 24.0) * nu * nu;
    double time_corr = 1.0 + (eps1 + eps2 + eps3) * T;

    return num_coeff * chi_z * time_corr;
}

double mco_sabr_atm_vol(double forward,
                        double time,
                        double alpha,
                        double beta,
                        double rho,
                        double nu)
{
    if (alpha < 1e-10 || forward <= 0.0) {
        return alpha;
    }

    double F = forward;
    double T = time;
    double one_beta = 1.0 - beta;

    /* F^(1-β) */
    double F_beta = pow(F, one_beta);

    /*
     * ATM Black vol = [α / F^(1-β)] · [1 + εT]
     *
     * where ε = [(1-β)²/24]·[α/F^(1-β)]² + [ρβνα/4F^(1-β)] + [(2-3ρ²)/24]·ν²
     */
    double alpha_adj = alpha / F_beta;

    double eps1 = (one_beta * one_beta / 24.0) * alpha_adj * alpha_adj;
    double eps2 = (rho * beta * nu / 4.0) * alpha_adj;
    double eps3 = ((2.0 - 3.0 * rho * rho) / 24.0) * nu * nu;

    double vol = alpha_adj * (1.0 + (eps1 + eps2 + eps3) * T);

    return vol;
}
