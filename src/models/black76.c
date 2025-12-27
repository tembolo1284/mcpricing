/*
 * Black-76 Model Implementation
 *
 * Analytical pricing and Greeks for futures/forward options.
 */

#include "internal/models/black76.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

/*
 * Standard normal CDF using erfc
 */
static inline double norm_cdf(double x)
{
    return 0.5 * erfc(-x / M_SQRT2);
}

/*
 * Standard normal PDF
 */
static inline double norm_pdf(double x)
{
    return exp(-0.5 * x * x) / sqrt(2.0 * M_PI);
}

/*============================================================================
 * Black-76 Analytical Pricing
 *============================================================================*/

double mco_black76_call(double forward,
                        double strike,
                        double rate,
                        double volatility,
                        double time)
{
    if (time <= 0.0) {
        return fmax(forward - strike, 0.0);
    }
    if (volatility <= 0.0) {
        return exp(-rate * time) * fmax(forward - strike, 0.0);
    }
    if (forward <= 0.0 || strike <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double d1 = (log(forward / strike) + 0.5 * volatility * volatility * time)
                / (volatility * sqrt_t);
    double d2 = d1 - volatility * sqrt_t;

    double df = exp(-rate * time);

    return df * (forward * norm_cdf(d1) - strike * norm_cdf(d2));
}

double mco_black76_put(double forward,
                       double strike,
                       double rate,
                       double volatility,
                       double time)
{
    if (time <= 0.0) {
        return fmax(strike - forward, 0.0);
    }
    if (volatility <= 0.0) {
        return exp(-rate * time) * fmax(strike - forward, 0.0);
    }
    if (forward <= 0.0 || strike <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double d1 = (log(forward / strike) + 0.5 * volatility * volatility * time)
                / (volatility * sqrt_t);
    double d2 = d1 - volatility * sqrt_t;

    double df = exp(-rate * time);

    return df * (strike * norm_cdf(-d2) - forward * norm_cdf(-d1));
}

/*============================================================================
 * Black-76 Greeks
 *============================================================================*/

double mco_black76_delta(double forward, double strike, double rate,
                         double volatility, double time, int is_call)
{
    if (time <= 0.0 || volatility <= 0.0) {
        if (is_call) {
            return (forward > strike) ? exp(-rate * time) : 0.0;
        } else {
            return (forward < strike) ? -exp(-rate * time) : 0.0;
        }
    }

    double sqrt_t = sqrt(time);
    double d1 = (log(forward / strike) + 0.5 * volatility * volatility * time)
                / (volatility * sqrt_t);
    double df = exp(-rate * time);

    if (is_call) {
        return df * norm_cdf(d1);
    } else {
        return df * (norm_cdf(d1) - 1.0);
    }
}

double mco_black76_gamma(double forward, double strike, double rate,
                         double volatility, double time)
{
    if (time <= 0.0 || volatility <= 0.0 || forward <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double d1 = (log(forward / strike) + 0.5 * volatility * volatility * time)
                / (volatility * sqrt_t);
    double df = exp(-rate * time);

    return df * norm_pdf(d1) / (forward * volatility * sqrt_t);
}

double mco_black76_vega(double forward, double strike, double rate,
                        double volatility, double time)
{
    if (time <= 0.0 || forward <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double d1;
    
    if (volatility <= 0.0) {
        d1 = (forward > strike) ? 1e10 : -1e10;
    } else {
        d1 = (log(forward / strike) + 0.5 * volatility * volatility * time)
             / (volatility * sqrt_t);
    }

    double df = exp(-rate * time);

    return df * forward * norm_pdf(d1) * sqrt_t;
}

double mco_black76_theta(double forward, double strike, double rate,
                         double volatility, double time, int is_call)
{
    if (time <= 0.0 || volatility <= 0.0) {
        return 0.0;
    }

    double sqrt_t = sqrt(time);
    double d1 = (log(forward / strike) + 0.5 * volatility * volatility * time)
                / (volatility * sqrt_t);
    double d2 = d1 - volatility * sqrt_t;
    double df = exp(-rate * time);

    double term1 = -forward * norm_pdf(d1) * volatility / (2.0 * sqrt_t);

    if (is_call) {
        double term2 = rate * forward * norm_cdf(d1);
        double term3 = -rate * strike * norm_cdf(d2);
        return df * (term1 + term2 + term3);
    } else {
        double term2 = rate * forward * norm_cdf(-d1);
        double term3 = -rate * strike * norm_cdf(-d2);
        return df * (term1 - term2 + term3);
    }
}

/*============================================================================
 * Black-76 Implied Volatility
 *============================================================================*/

double mco_black76_implied_vol(double forward,
                               double strike,
                               double rate,
                               double time,
                               double price,
                               int is_call)
{
    if (time <= 0.0 || price <= 0.0) {
        return 0.0;
    }

    /* Initial guess using Brenner-Subrahmanyam approximation */
    double df = exp(-rate * time);
    double sigma = sqrt(2.0 * M_PI / time) * price / (df * forward);

    /* Newton-Raphson iteration */
    for (int i = 0; i < 50; ++i) {
        double model_price = is_call ? mco_black76_call(forward, strike, rate, sigma, time)
                                     : mco_black76_put(forward, strike, rate, sigma, time);
        double vega = mco_black76_vega(forward, strike, rate, sigma, time);

        if (vega < 1e-12) break;

        double diff = model_price - price;
        if (fabs(diff) < 1e-10) break;

        sigma -= diff / vega;
        if (sigma <= 0.0) sigma = 0.001;
        if (sigma > 5.0) sigma = 5.0;
    }

    return sigma;
}
