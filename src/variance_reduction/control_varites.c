/*
 * Control Variates Implementation
 */

#include "internal/variance_reduction/control_variates.h"
#include "internal/models/gbm.h"
#include "internal/instruments/asian.h"
#include "internal/allocator.h"
#include "mcoptions.h"
#include <math.h>

/*============================================================================
 * European with Spot Control Variate
 *============================================================================*/

double mco_european_cv_spot(mco_ctx *ctx,
                            double spot,
                            double strike,
                            double rate,
                            double volatility,
                            double time_to_maturity,
                            mco_option_type type)
{
    if (!ctx) return 0.0;

    uint64_t n_paths = ctx->num_simulations;

    /* Initialize GBM model */
    mco_gbm model;
    mco_gbm_init(&model, spot, rate, volatility, time_to_maturity);

    /* E[S(T)] = S(0)·e^(rT) - the known expectation of our control */
    double ez = spot * exp(rate * time_to_maturity);

    /* Initialize control variate statistics */
    mco_cv_stats stats;
    mco_cv_init(&stats, ez);

    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        /* Simulate terminal spot */
        double s_t = mco_gbm_simulate(&model, &rng);

        /* Primary estimator: discounted payoff */
        double payoff = mco_payoff(s_t, strike, type);
        double x = model.discount * payoff;

        /* Control variate: terminal spot */
        double z = s_t;

        mco_cv_add(&stats, x, z);
    }

    return mco_cv_estimate(&stats);
}

/*============================================================================
 * Arithmetic Asian with Geometric Asian Control Variate
 *============================================================================*/

double mco_asian_cv_geometric(mco_ctx *ctx,
                              double spot,
                              double strike,
                              double rate,
                              double volatility,
                              double time_to_maturity,
                              size_t num_obs,
                              mco_option_type type)
{
    if (!ctx || num_obs == 0) return 0.0;

    uint64_t n_paths = ctx->num_simulations;

    /* Allocate path storage */
    double *path = (double *)mco_malloc((num_obs + 1) * sizeof(double));
    if (!path) {
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Initialize GBM path model */
    mco_gbm_path model;
    mco_gbm_path_init(&model, spot, rate, volatility, time_to_maturity, num_obs);

    /* E[geometric Asian payoff] - computed analytically */
    double ez = mco_asian_geometric_closed(spot, strike, rate, volatility,
                                            time_to_maturity, num_obs, type);

    /* Initialize control variate statistics */
    mco_cv_stats stats;
    mco_cv_init(&stats, ez);

    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        /* Simulate path */
        mco_gbm_simulate_path(&model, &rng, path);

        /* Compute arithmetic average (skip path[0]) */
        double arith_sum = 0.0;
        double log_sum = 0.0;

        for (size_t j = 1; j <= num_obs; ++j) {
            arith_sum += path[j];
            log_sum += log(path[j]);
        }

        double arith_avg = arith_sum / (double)num_obs;
        double geom_avg = exp(log_sum / (double)num_obs);

        /* Primary: arithmetic Asian payoff (discounted) */
        double x = model.discount * mco_payoff(arith_avg, strike, type);

        /* Control: geometric Asian payoff (discounted) */
        double z = model.discount * mco_payoff(geom_avg, strike, type);

        mco_cv_add(&stats, x, z);
    }

    mco_free(path);

    return mco_cv_estimate(&stats);
}

/*============================================================================
 * Public API Wrappers
 *============================================================================*/

double mco_european_call_cv(mco_ctx *ctx,
                            double spot,
                            double strike,
                            double rate,
                            double volatility,
                            double time_to_maturity)
{
    return mco_european_cv_spot(ctx, spot, strike, rate, volatility,
                                time_to_maturity, MCO_CALL);
}

double mco_european_put_cv(mco_ctx *ctx,
                           double spot,
                           double strike,
                           double rate,
                           double volatility,
                           double time_to_maturity)
{
    return mco_european_cv_spot(ctx, spot, strike, rate, volatility,
                                time_to_maturity, MCO_PUT);
}

double mco_asian_call_cv(mco_ctx *ctx,
                         double spot,
                         double strike,
                         double rate,
                         double volatility,
                         double time_to_maturity,
                         size_t num_obs)
{
    return mco_asian_cv_geometric(ctx, spot, strike, rate, volatility,
                                   time_to_maturity, num_obs, MCO_CALL);
}

double mco_asian_put_cv(mco_ctx *ctx,
                        double spot,
                        double strike,
                        double rate,
                        double volatility,
                        double time_to_maturity,
                        size_t num_obs)
{
    return mco_asian_cv_geometric(ctx, spot, strike, rate, volatility,
                                   time_to_maturity, num_obs, MCO_PUT);
}
