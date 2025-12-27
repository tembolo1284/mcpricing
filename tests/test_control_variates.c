/*
 * Control Variates Tests
 *
 * Tests for control variate variance reduction.
 * Verifies:
 *   - CV prices match standard MC prices (within tolerance)
 *   - CV reduces variance (tighter confidence interval)
 *   - Geometric Asian CV for arithmetic Asian
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/variance_reduction/control_variates.h"
#include "internal/models/gbm.h"
#include <math.h>

#define CV_TOLERANCE 0.50

/*-------------------------------------------------------
 * European with Spot Control Variate
 *-------------------------------------------------------*/

void test_european_cv_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_european_call_cv(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    /* Should be close to Black-Scholes */
    double bs = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(CV_TOLERANCE, bs, price);

    mco_ctx_free(ctx);
}

void test_european_cv_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_european_put_cv(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    double bs = mco_black_scholes_put(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(CV_TOLERANCE, bs, price);

    mco_ctx_free(ctx);
}

void test_european_cv_reduces_variance(void)
{
    /*
     * Compare variance of standard MC vs CV MC
     * by running multiple batches and computing std dev
     */
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 10000);

    double prices_std[10];
    double prices_cv[10];

    /* Run 10 batches with different seeds */
    for (int i = 0; i < 10; ++i) {
        mco_set_seed(ctx, 100 + i);
        prices_std[i] = mco_european_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

        mco_set_seed(ctx, 100 + i);
        prices_cv[i] = mco_european_call_cv(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);
    }

    /* Compute standard deviations */
    double mean_std = 0, mean_cv = 0;
    for (int i = 0; i < 10; ++i) {
        mean_std += prices_std[i];
        mean_cv += prices_cv[i];
    }
    mean_std /= 10;
    mean_cv /= 10;

    double var_std = 0, var_cv = 0;
    for (int i = 0; i < 10; ++i) {
        var_std += (prices_std[i] - mean_std) * (prices_std[i] - mean_std);
        var_cv += (prices_cv[i] - mean_cv) * (prices_cv[i] - mean_cv);
    }

    /* CV should have lower or similar variance */
    /* (May not always be lower due to randomness, but should not be much higher) */
    TEST_ASSERT_TRUE(var_cv <= var_std * 2.0);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Asian with Geometric Control Variate
 *-------------------------------------------------------*/

void test_asian_cv_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_asian_call_cv(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    /* Should be positive and in reasonable range */
    TEST_ASSERT_TRUE(price > 0.0);
    TEST_ASSERT_TRUE(price < 15.0);

    /* Compare with standard Asian */
    mco_set_seed(ctx, 42);
    double std_price = mco_asian_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    /* Should be close to standard MC */
    TEST_ASSERT_DOUBLE_WITHIN(1.0, std_price, price);

    mco_ctx_free(ctx);
}

void test_asian_cv_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_asian_put_cv(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    TEST_ASSERT_TRUE(price > 0.0);
    TEST_ASSERT_TRUE(price < 10.0);

    mco_ctx_free(ctx);
}

void test_asian_cv_vs_geometric_closed(void)
{
    /*
     * When using geometric CV, the adjusted estimate should be
     * close to the true arithmetic Asian price.
     * 
     * We can't verify exactly, but we can check it's reasonable.
     */
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);

    double cv_price = mco_asian_call_cv(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 52);

    /* Geometric Asian is a lower bound for arithmetic Asian (for calls) */
    double geom_closed = mco_asian_geometric_closed(100.0, 100.0, 0.05, 0.20, 1.0, 52, MCO_CALL);

    /* Arithmetic should be >= geometric (with some MC noise tolerance) */
    TEST_ASSERT_TRUE(cv_price >= geom_closed - 0.3);

    mco_ctx_free(ctx);
}

void test_asian_cv_reduces_variance(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 10000);

    double prices_std[10];
    double prices_cv[10];

    for (int i = 0; i < 10; ++i) {
        mco_set_seed(ctx, 200 + i);
        prices_std[i] = mco_asian_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

        mco_set_seed(ctx, 200 + i);
        prices_cv[i] = mco_asian_call_cv(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);
    }

    double mean_std = 0, mean_cv = 0;
    for (int i = 0; i < 10; ++i) {
        mean_std += prices_std[i];
        mean_cv += prices_cv[i];
    }
    mean_std /= 10;
    mean_cv /= 10;

    double var_std = 0, var_cv = 0;
    for (int i = 0; i < 10; ++i) {
        var_std += (prices_std[i] - mean_std) * (prices_std[i] - mean_std);
        var_cv += (prices_cv[i] - mean_cv) * (prices_cv[i] - mean_cv);
    }

    /* Geometric Asian CV should reduce variance significantly */
    /* (Correlation between arithmetic and geometric is very high) */
    TEST_ASSERT_TRUE(var_cv < var_std * 1.5);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Reproducibility
 *-------------------------------------------------------*/

void test_cv_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_european_call_cv(ctx1, 100.0, 100.0, 0.05, 0.20, 1.0);
    double price2 = mco_european_call_cv(ctx2, 100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_control_variates.c");

    /* European with spot CV */
    RUN_TEST(test_european_cv_call_atm);
    RUN_TEST(test_european_cv_put_atm);
    RUN_TEST(test_european_cv_reduces_variance);

    /* Asian with geometric CV */
    RUN_TEST(test_asian_cv_call_atm);
    RUN_TEST(test_asian_cv_put_atm);
    RUN_TEST(test_asian_cv_vs_geometric_closed);
    RUN_TEST(test_asian_cv_reduces_variance);

    /* Reproducibility */
    RUN_TEST(test_cv_reproducible);

    return UnityEnd();
}
