/*
 * Geometric Brownian Motion - Black-Scholes Implementation
 *
 * The closed-form Black-Scholes formula is used for:
 *   1. Validation - compare MC results against analytical
 *   2. Control variates - reduce variance using known solution
 *
 * Formula:
 *   Call: S·N(d1) - K·e^(-rT)·N(d2)
 *   Put:  K·e^(-rT)·N(-d2) - S·N(-d1)
 *
 * Where:
 *   d1 = (ln(S/K) + (r + σ²/2)T) / (σ√T)
 *   d2 = d1 - σ√T
 *   N(x) = CDF of standard normal
 */

#include "internal/models/gbm.h"
#include <math.h>

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.7071067811865475244  /* 1/sqrt(2) */
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390  /* 2/sqrt(pi) */
#endif

/*============================================================================
 * Standard Normal CDF
 *============================================================================*/

/*
 * Standard normal CDF using the complementary error function.
 *
 * N(x) = (1 + erf(x / sqrt(2))) / 2
 *
 * We use erfc for better numerical stability in the tails:
 * N(x) = erfc(-x / sqrt(2)) / 2
 */
static double norm_cdf(double x)
{
    return 0.5 * erfc(-x * M_SQRT1_2);
}

/*============================================================================
 * Black-Scholes Formulas
 *============================================================================*/

double mco_black_scholes_call(double spot, double strike, double rate,
                              double volatility, double time)
{
    /* Handle edge cases */
    if (time <= 0.0) {
        return fmax(spot - strike, 0.0);
    }
    if (volatility <= 0.0) {
        double df = exp(-rate * time);
        return fmax(spot - strike * df, 0.0);
    }
    if (strike <= 0.0) {
        return spot;
    }

    double sqrt_t = sqrt(time);
    double vol_sqrt_t = volatility * sqrt_t;

    double d1 = (log(spot / strike) + (rate + 0.5 * volatility * volatility) * time) 
                / vol_sqrt_t;
    double d2 = d1 - vol_sqrt_t;

    double df = exp(-rate * time);

    return spot * norm_cdf(d1) - strike * df * norm_cdf(d2);
}

double mco_black_scholes_put(double spot, double strike, double rate,
                             double volatility, double time)
{
    /* Handle edge cases */
    if (time <= 0.0) {
        return fmax(strike - spot, 0.0);
    }
    if (volatility <= 0.0) {
        double df = exp(-rate * time);
        return fmax(strike * df - spot, 0.0);
    }
    if (strike <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double vol_sqrt_t = volatility * sqrt_t;

    double d1 = (log(spot / strike) + (rate + 0.5 * volatility * volatility) * time) 
                / vol_sqrt_t;
    double d2 = d1 - vol_sqrt_t;

    double df = exp(-rate * time);

    return strike * df * norm_cdf(-d2) - spot * norm_cdf(-d1);
}
