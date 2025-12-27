/*
 * SABR European Option Pricing
 *
 * Monte Carlo pricing under SABR dynamics.
 */

#include "internal/models/sabr.h"
#include "internal/instruments/payoff.h"
#include "internal/context.h"
#include "internal/allocator.h"
#include "mcoptions.h"
#include <math.h>

/*============================================================================
 * Context SABR Parameter Setting
 *============================================================================*/

void mco_set_sabr(mco_ctx *ctx, double alpha, double beta, double rho, double nu)
{
    if (!ctx) return;

    ctx->sabr_alpha = alpha;
    ctx->sabr_beta  = beta;
    ctx->sabr_rho   = rho;
    ctx->sabr_nu    = nu;
    ctx->model      = 2;  /* SABR model ID */
}

/*============================================================================
 * SABR European Option Pricing
 *============================================================================*/

static double price_sabr_european(mco_ctx *ctx,
                                  double forward,
                                  double strike,
                                  double rate,
                                  double time_to_maturity,
                                  double alpha,
                                  double beta,
                                  double rho,
                                  double nu,
                                  mco_option_type type)
{
    if (!ctx) return 0.0;

    if (forward <= 0.0 || strike <= 0.0 || time_to_maturity < 0.0) {
        ctx->last_error = MCO_ERR_INVALID_ARG;
        return 0.0;
    }

    /* Use more steps for SABR due to stochastic vol */
    size_t num_steps = ctx->num_steps;
    if (num_steps < 100) num_steps = 100;

    uint64_t n_paths = ctx->num_simulations;

    /* Initialize SABR path model */
    mco_sabr_path model;
    mco_sabr_path_init(&model, forward, alpha, beta, rho, nu,
                        time_to_maturity, rate, num_steps);

    double sum_payoff = 0.0;
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        double f_T = mco_sabr_simulate_terminal(&model, &rng);
        double payoff = mco_payoff(f_T, strike, type);
        sum_payoff += payoff;
    }

    double price = model.discount * (sum_payoff / (double)n_paths);
    return price;
}

double mco_sabr_european_call(mco_ctx *ctx,
                              double forward,
                              double strike,
                              double rate,
                              double time_to_maturity,
                              double alpha,
                              double beta,
                              double rho,
                              double nu)
{
    return price_sabr_european(ctx, forward, strike, rate, time_to_maturity,
                               alpha, beta, rho, nu, MCO_CALL);
}

double mco_sabr_european_put(mco_ctx *ctx,
                             double forward,
                             double strike,
                             double rate,
                             double time_to_maturity,
                             double alpha,
                             double beta,
                             double rho,
                             double nu)
{
    return price_sabr_european(ctx, forward, strike, rate, time_to_maturity,
                               alpha, beta, rho, nu, MCO_PUT);
}
