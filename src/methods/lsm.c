/*
 * Least Squares Monte Carlo Implementation
 *
 * Longstaff-Schwartz algorithm for American option pricing.
 */

#include "internal/methods/lsm.h"
#include "internal/models/gbm.h"
#include "internal/allocator.h"
#include "internal/rng.h"
#include <math.h>
#include <string.h>

/*============================================================================
 * Least Squares Regression (Normal Equations)
 *============================================================================*/

/*
 * Solve the normal equations (A'A)x = A'b for least squares.
 *
 * For our small 3x3 system, we use direct computation rather than
 * a general-purpose solver. This is faster and avoids dependencies.
 */
int mco_lsm_regress(const double *A,
                    const double *b,
                    double *coeffs,
                    size_t n_samples,
                    size_t n_basis)
{
    /* Compute A'A (n_basis × n_basis) and A'b (n_basis) */
    double AtA[MCO_LSM_NUM_BASIS * MCO_LSM_NUM_BASIS] = {0};
    double Atb[MCO_LSM_NUM_BASIS] = {0};

    for (size_t i = 0; i < n_samples; ++i) {
        const double *row = A + i * n_basis;
        double bi = b[i];

        for (size_t j = 0; j < n_basis; ++j) {
            Atb[j] += row[j] * bi;
            for (size_t k = 0; k < n_basis; ++k) {
                AtA[j * n_basis + k] += row[j] * row[k];
            }
        }
    }

    /*
     * Solve 3x3 system using Gaussian elimination with partial pivoting.
     * For production, consider using LAPACK, but this works for small systems.
     */
    double aug[MCO_LSM_NUM_BASIS][MCO_LSM_NUM_BASIS + 1];

    /* Build augmented matrix [AtA | Atb] */
    for (size_t i = 0; i < n_basis; ++i) {
        for (size_t j = 0; j < n_basis; ++j) {
            aug[i][j] = AtA[i * n_basis + j];
        }
        aug[i][n_basis] = Atb[i];
    }

    /* Forward elimination with partial pivoting */
    for (size_t col = 0; col < n_basis; ++col) {
        /* Find pivot */
        size_t max_row = col;
        double max_val = fabs(aug[col][col]);
        for (size_t row = col + 1; row < n_basis; ++row) {
            if (fabs(aug[row][col]) > max_val) {
                max_val = fabs(aug[row][col]);
                max_row = row;
            }
        }

        /* Swap rows if needed */
        if (max_row != col) {
            for (size_t j = 0; j <= n_basis; ++j) {
                double tmp = aug[col][j];
                aug[col][j] = aug[max_row][j];
                aug[max_row][j] = tmp;
            }
        }

        /* Check for singularity */
        if (fabs(aug[col][col]) < 1e-12) {
            /* Matrix is singular - return zeros */
            for (size_t i = 0; i < n_basis; ++i) {
                coeffs[i] = 0.0;
            }
            return -1;
        }

        /* Eliminate below */
        for (size_t row = col + 1; row < n_basis; ++row) {
            double factor = aug[row][col] / aug[col][col];
            for (size_t j = col; j <= n_basis; ++j) {
                aug[row][j] -= factor * aug[col][j];
            }
        }
    }

    /* Back substitution */
    for (int i = (int)n_basis - 1; i >= 0; --i) {
        coeffs[i] = aug[i][n_basis];
        for (size_t j = (size_t)i + 1; j < n_basis; ++j) {
            coeffs[i] -= aug[i][j] * coeffs[j];
        }
        coeffs[i] /= aug[i][i];
    }

    return 0;
}

/*============================================================================
 * LSM American Option Pricing
 *============================================================================*/

double mco_lsm_american(mco_ctx *ctx,
                        double spot,
                        double strike,
                        double rate,
                        double volatility,
                        double time_to_maturity,
                        size_t num_steps,
                        mco_option_type type)
{
    if (!ctx || num_steps == 0) return 0.0;

    uint64_t n_paths = ctx->num_simulations;
    double dt = time_to_maturity / (double)num_steps;
    double df = exp(-rate * dt);  /* Per-step discount factor */

    /* Allocate memory for paths and cash flows */
    /* paths[i * (num_steps + 1) + j] = spot price of path i at step j */
    double *paths = (double *)mco_malloc(n_paths * (num_steps + 1) * sizeof(double));
    /* cashflow[i] = present value of optimal exercise for path i */
    double *cashflow = (double *)mco_calloc(n_paths, sizeof(double));
    /* exercise_time[i] = step at which path i is exercised (num_steps = at maturity) */
    size_t *exercise_time = (size_t *)mco_malloc(n_paths * sizeof(size_t));

    if (!paths || !cashflow || !exercise_time) {
        mco_free(paths);
        mco_free(cashflow);
        mco_free(exercise_time);
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Initialize GBM path model */
    mco_gbm_path model;
    mco_gbm_path_init(&model, spot, rate, volatility, time_to_maturity, num_steps);

    /*=========================================================================
     * Step 1: Generate all paths forward
     *=========================================================================*/
    mco_rng rng = ctx->rng;  /* Copy RNG state */

    for (uint64_t i = 0; i < n_paths; ++i) {
        double *path = paths + i * (num_steps + 1);
        mco_gbm_simulate_path(&model, &rng, path);
        exercise_time[i] = num_steps;  /* Default: exercise at maturity */
    }

    /*=========================================================================
     * Step 2: Initialize with terminal payoffs
     *=========================================================================*/
    for (uint64_t i = 0; i < n_paths; ++i) {
        double s_T = paths[i * (num_steps + 1) + num_steps];
        cashflow[i] = mco_payoff(s_T, strike, type);
    }

    /*=========================================================================
     * Step 3: Backward induction with regression
     *=========================================================================*/
    
    /* Temporary arrays for regression */
    double *A = (double *)mco_malloc(n_paths * MCO_LSM_NUM_BASIS * sizeof(double));
    double *Y = (double *)mco_malloc(n_paths * sizeof(double));
    uint64_t *itm_indices = (uint64_t *)mco_malloc(n_paths * sizeof(uint64_t));

    if (!A || !Y || !itm_indices) {
        mco_free(paths);
        mco_free(cashflow);
        mco_free(exercise_time);
        mco_free(A);
        mco_free(Y);
        mco_free(itm_indices);
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Work backwards from step (num_steps - 1) to step 1 */
    for (size_t step = num_steps - 1; step >= 1; --step) {
        /*
         * Discount all cash flows one step back
         */
        for (uint64_t i = 0; i < n_paths; ++i) {
            cashflow[i] *= df;
        }

        /*
         * Find in-the-money paths at this step
         */
        size_t n_itm = 0;
        for (uint64_t i = 0; i < n_paths; ++i) {
            double s_t = paths[i * (num_steps + 1) + step];
            double exercise_value = mco_payoff(s_t, strike, type);

            if (exercise_value > 0.0) {
                itm_indices[n_itm] = i;

                /* Build design matrix row: basis functions of S/K */
                double x = s_t / strike;
                mco_lsm_basis(x, A + n_itm * MCO_LSM_NUM_BASIS);

                /* Target: discounted future cashflow */
                Y[n_itm] = cashflow[i];

                n_itm++;
            }
        }

        /* Need at least as many samples as basis functions */
        if (n_itm < MCO_LSM_NUM_BASIS) {
            continue;
        }

        /*
         * Regression: estimate continuation value
         */
        double coeffs[MCO_LSM_NUM_BASIS];
        int rc = mco_lsm_regress(A, Y, coeffs, n_itm, MCO_LSM_NUM_BASIS);

        if (rc != 0) {
            /* Regression failed - skip this step */
            continue;
        }

        /*
         * Exercise decision: compare immediate vs continuation
         */
        for (size_t j = 0; j < n_itm; ++j) {
            uint64_t i = itm_indices[j];
            double s_t = paths[i * (num_steps + 1) + step];
            double exercise_value = mco_payoff(s_t, strike, type);

            /* Estimated continuation value from regression */
            double x = s_t / strike;
            double basis[MCO_LSM_NUM_BASIS];
            mco_lsm_basis(x, basis);

            double continuation = 0.0;
            for (size_t k = 0; k < MCO_LSM_NUM_BASIS; ++k) {
                continuation += coeffs[k] * basis[k];
            }

            /* Exercise if immediate value exceeds continuation */
            if (exercise_value > continuation) {
                cashflow[i] = exercise_value;
                exercise_time[i] = step;
            }
        }
    }

    /*=========================================================================
     * Step 4: Discount remaining cash flows to time 0 and average
     *=========================================================================*/
    
    /* Final discount to time 0 */
    for (uint64_t i = 0; i < n_paths; ++i) {
        cashflow[i] *= df;  /* One more step */
    }

    double sum = 0.0;
    for (uint64_t i = 0; i < n_paths; ++i) {
        sum += cashflow[i];
    }

    double price = sum / (double)n_paths;

    /* Cleanup */
    mco_free(paths);
    mco_free(cashflow);
    mco_free(exercise_time);
    mco_free(A);
    mco_free(Y);
    mco_free(itm_indices);

    return price;
}
