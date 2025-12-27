/*
 * Bermudan Option Implementation
 *
 * Uses LSM with exercise only at specified dates.
 */

#include "internal/instruments/bermudan.h"
#include "internal/methods/lsm.h"
#include "internal/models/gbm.h"
#include "internal/allocator.h"
#include "mcoptions.h"
#include <math.h>
#include <string.h>

/*============================================================================
 * Bermudan LSM Implementation
 *============================================================================*/

double mco_price_bermudan(mco_ctx *ctx,
                          double spot,
                          double strike,
                          double rate,
                          double volatility,
                          double time_to_maturity,
                          const double *exercise_times,
                          size_t num_exercise,
                          mco_option_type type)
{
    if (!ctx || !exercise_times || num_exercise == 0) return 0.0;

    if (spot <= 0.0 || strike <= 0.0 || volatility < 0.0 || time_to_maturity < 0.0) {
        ctx->last_error = MCO_ERR_INVALID_ARG;
        return 0.0;
    }

    uint64_t n_paths = ctx->num_simulations;

    /*
     * We need to simulate paths and store spot prices at each exercise date.
     * Use fine time steps for accuracy, but only record at exercise times.
     */
    size_t sim_steps = num_exercise * 10;  /* 10 steps between each exercise */
    if (sim_steps < 50) sim_steps = 50;

    /* Allocate arrays */
    double *spot_at_ex = (double *)mco_malloc(n_paths * num_exercise * sizeof(double));
    double *cashflow = (double *)mco_calloc(n_paths, sizeof(double));
    double *path = (double *)mco_malloc((sim_steps + 1) * sizeof(double));

    /* Regression arrays */
    double *A = (double *)mco_malloc(n_paths * MCO_LSM_NUM_BASIS * sizeof(double));
    double *Y = (double *)mco_malloc(n_paths * sizeof(double));
    uint64_t *itm_indices = (uint64_t *)mco_malloc(n_paths * sizeof(uint64_t));

    if (!spot_at_ex || !cashflow || !path || !A || !Y || !itm_indices) {
        mco_free(spot_at_ex);
        mco_free(cashflow);
        mco_free(path);
        mco_free(A);
        mco_free(Y);
        mco_free(itm_indices);
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Initialize GBM path model */
    mco_gbm_path model;
    mco_gbm_path_init(&model, spot, rate, volatility, time_to_maturity, sim_steps);

    /* Convert exercise times to step indices */
    size_t *ex_steps = (size_t *)mco_malloc(num_exercise * sizeof(size_t));
    if (!ex_steps) {
        mco_free(spot_at_ex);
        mco_free(cashflow);
        mco_free(path);
        mco_free(A);
        mco_free(Y);
        mco_free(itm_indices);
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    for (size_t i = 0; i < num_exercise; ++i) {
        /* exercise_times[i] is fraction of time_to_maturity */
        double t_frac = exercise_times[i];
        if (t_frac > 1.0) t_frac = 1.0;
        if (t_frac < 0.0) t_frac = 0.0;
        ex_steps[i] = (size_t)(t_frac * sim_steps + 0.5);
        if (ex_steps[i] > sim_steps) ex_steps[i] = sim_steps;
    }

    /*=========================================================================
     * Step 1: Simulate paths and record spot at exercise dates
     *=========================================================================*/
    mco_rng rng = ctx->rng;

    for (uint64_t i = 0; i < n_paths; ++i) {
        mco_gbm_simulate_path(&model, &rng, path);

        for (size_t j = 0; j < num_exercise; ++j) {
            spot_at_ex[i * num_exercise + j] = path[ex_steps[j]];
        }
    }

    /*=========================================================================
     * Step 2: Initialize with terminal payoffs
     *=========================================================================*/
    for (uint64_t i = 0; i < n_paths; ++i) {
        double s_T = spot_at_ex[i * num_exercise + (num_exercise - 1)];
        cashflow[i] = mco_payoff(s_T, strike, type);
    }

    /*=========================================================================
     * Step 3: Backward induction through exercise dates
     *=========================================================================*/
    for (int ex_idx = (int)num_exercise - 2; ex_idx >= 0; --ex_idx) {
        /* Discount factor from this exercise to next */
        double t_this = exercise_times[ex_idx] * time_to_maturity;
        double t_next = exercise_times[ex_idx + 1] * time_to_maturity;
        double df = exp(-rate * (t_next - t_this));

        /* Discount cashflows one period */
        for (uint64_t i = 0; i < n_paths; ++i) {
            cashflow[i] *= df;
        }

        /* Find ITM paths */
        size_t n_itm = 0;
        for (uint64_t i = 0; i < n_paths; ++i) {
            double s_t = spot_at_ex[i * num_exercise + ex_idx];
            double ex_val = mco_payoff(s_t, strike, type);

            if (ex_val > 0.0) {
                itm_indices[n_itm] = i;

                double x = s_t / strike;
                mco_lsm_basis(x, A + n_itm * MCO_LSM_NUM_BASIS);
                Y[n_itm] = cashflow[i];

                n_itm++;
            }
        }

        if (n_itm < MCO_LSM_NUM_BASIS) continue;

        /* Regression */
        double coeffs[MCO_LSM_NUM_BASIS];
        int rc = mco_lsm_regress(A, Y, coeffs, n_itm, MCO_LSM_NUM_BASIS);
        if (rc != 0) continue;

        /* Exercise decision */
        for (size_t j = 0; j < n_itm; ++j) {
            uint64_t i = itm_indices[j];
            double s_t = spot_at_ex[i * num_exercise + ex_idx];
            double ex_val = mco_payoff(s_t, strike, type);

            double x = s_t / strike;
            double basis[MCO_LSM_NUM_BASIS];
            mco_lsm_basis(x, basis);

            double continuation = 0.0;
            for (size_t k = 0; k < MCO_LSM_NUM_BASIS; ++k) {
                continuation += coeffs[k] * basis[k];
            }

            if (ex_val > continuation) {
                cashflow[i] = ex_val;
            }
        }
    }

    /*=========================================================================
     * Step 4: Final discount to time 0
     *=========================================================================*/
    double t_first = exercise_times[0] * time_to_maturity;
    double df_first = exp(-rate * t_first);

    double sum = 0.0;
    for (uint64_t i = 0; i < n_paths; ++i) {
        sum += cashflow[i] * df_first;
    }

    double price = sum / (double)n_paths;

    /* Cleanup */
    mco_free(spot_at_ex);
    mco_free(cashflow);
    mco_free(path);
    mco_free(A);
    mco_free(Y);
    mco_free(itm_indices);
    mco_free(ex_steps);

    return price;
}

double mco_price_bermudan_uniform(mco_ctx *ctx,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity,
                                  size_t num_exercise,
                                  mco_option_type type)
{
    if (num_exercise == 0) return 0.0;

    /* Create uniform exercise times */
    double *ex_times = (double *)mco_malloc(num_exercise * sizeof(double));
    if (!ex_times) {
        if (ctx) ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    for (size_t i = 0; i < num_exercise; ++i) {
        ex_times[i] = (double)(i + 1) / (double)num_exercise;
    }

    double price = mco_price_bermudan(ctx, spot, strike, rate, volatility,
                                       time_to_maturity, ex_times, num_exercise, type);

    mco_free(ex_times);
    return price;
}

/*============================================================================
 * Public API
 *============================================================================*/

double mco_bermudan_call(mco_ctx *ctx,
                         double spot,
                         double strike,
                         double rate,
                         double volatility,
                         double time_to_maturity,
                         size_t num_exercise)
{
    return mco_price_bermudan_uniform(ctx, spot, strike, rate, volatility,
                                       time_to_maturity, num_exercise, MCO_CALL);
}

double mco_bermudan_put(mco_ctx *ctx,
                        double spot,
                        double strike,
                        double rate,
                        double volatility,
                        double time_to_maturity,
                        size_t num_exercise)
{
    return mco_price_bermudan_uniform(ctx, spot, strike, rate, volatility,
                                       time_to_maturity, num_exercise, MCO_PUT);
}
