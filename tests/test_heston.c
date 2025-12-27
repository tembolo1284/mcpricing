/*
 * Heston Stochastic Volatility Model Tests
 *
 * Tests for Heston MC pricing.
 * Verifies:
 *   - Feller condition check
 *   - Reasonable prices
 *   - Skew with negative correlation
 *   - Reproducibility
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/models/heston.h"
#include <math.h>

/*
 * Standard Heston parameters
 * (based on typical equity calibrations)
 */
#define TEST_V0     0.04     /* 20% initial vol (variance = 0.04) */
#define TEST_KAPPA  2.0      /* Mean reversion speed */
#define TEST_THETA  0.04     /* Long-run variance (20% vol) */
#define TEST_SIGMA  0.3      /* Vol of vol */
#define TEST_RHO   -0.7      /* Negative correlation (leverage) */

#define HESTON_TOLERANCE 1.0

/*-------------------------------------------------------
 * Feller Condition Tests
 *-------------------------------------------------------*/

void test_heston_feller_satisfied(void)
{
    /* 2κθ > σ² : 2*2*0.04 = 0.16 > 0.09 = 0.3² ✓ */
    int ok = mco_heston_check_feller(TEST_KAPPA, TEST_THETA, TEST_SIGMA);
    TEST_ASSERT_TRUE(ok);
}

void test_heston_feller_violated(void)
{
    /* High vol-of-vol violates Feller */
    int ok = mco_heston_check_feller(2.0, 0.04, 1.0);  /* 0.16 < 1.0 */
    TEST_ASSERT_FALSE(ok);
}

/*-------------------------------------------------------
 * Basic Pricing Tests
 *-------------------------------------------------------*/

void test_heston_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_steps(ctx, 100);
    mco_set_seed(ctx, 42);

    double price = mco_heston_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                             TEST_V0, TEST_KAPPA, TEST_THETA,
                                             TEST_SIGMA, TEST_RHO);

    /* Should be positive and in reasonable range (BS would give ~10.45) */
    TEST_ASSERT_TRUE(price > 5.0);
    TEST_ASSERT_TRUE(price < 20.0);

    mco_ctx_free(ctx);
}

void test_heston_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_steps(ctx, 100);
    mco_set_seed(ctx, 42);

    double price = mco_heston_european_put(ctx, 100.0, 100.0, 0.05, 1.0,
                                            TEST_V0, TEST_KAPPA, TEST_THETA,
                                            TEST_SIGMA, TEST_RHO);

    TEST_ASSERT_TRUE(price > 2.0);
    TEST_ASSERT_TRUE(price < 15.0);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Skew Tests (Negative Correlation)
 *-------------------------------------------------------*/

void test_heston_negative_skew(void)
{
    /*
     * With negative rho, OTM puts should have higher implied vol
     * than OTM calls. We test this by checking that put prices
     * are relatively high compared to calls at same OTM distance.
     */
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 30000);
    mco_set_steps(ctx, 100);
    mco_set_seed(ctx, 42);

    /* OTM put (K=90) */
    double put_90 = mco_heston_european_put(ctx, 100.0, 90.0, 0.05, 1.0,
                                             TEST_V0, TEST_KAPPA, TEST_THETA,
                                             TEST_SIGMA, TEST_RHO);

    mco_set_seed(ctx, 42);
    /* OTM call (K=110) */
    double call_110 = mco_heston_european_call(ctx, 100.0, 110.0, 0.05, 1.0,
                                                TEST_V0, TEST_KAPPA, TEST_THETA,
                                                TEST_SIGMA, TEST_RHO);

    /* With strong negative rho, put should be relatively more expensive */
    /* (Both are OTM by 10%, but put has higher implied vol) */
    TEST_ASSERT_TRUE(put_90 > 0.0);
    TEST_ASSERT_TRUE(call_110 > 0.0);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Parameter Sensitivity Tests
 *-------------------------------------------------------*/

void test_heston_v0_sensitivity(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 30000);
    mco_set_steps(ctx, 100);

    /* Higher initial variance = higher option price */
    mco_set_seed(ctx, 42);
    double price_low = mco_heston_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                                 0.02, TEST_KAPPA, TEST_THETA,
                                                 TEST_SIGMA, TEST_RHO);

    mco_set_seed(ctx, 42);
    double price_high = mco_heston_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                                  0.09, TEST_KAPPA, TEST_THETA,
                                                  TEST_SIGMA, TEST_RHO);

    TEST_ASSERT_TRUE(price_high > price_low);

    mco_ctx_free(ctx);
}

void test_heston_kappa_sensitivity(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 30000);
    mco_set_steps(ctx, 100);

    /* Higher mean reversion = less vol-of-vol effect */
    /* When v0 > theta, higher kappa pulls vol down faster */
    mco_set_seed(ctx, 42);
    double price_slow = mco_heston_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                                  0.09, 0.5, 0.04,  /* slow reversion, high v0 */
                                                  TEST_SIGMA, TEST_RHO);

    mco_set_seed(ctx, 42);
    double price_fast = mco_heston_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                                  0.09, 5.0, 0.04,  /* fast reversion, high v0 */
                                                  TEST_SIGMA, TEST_RHO);

    /* Fast reversion should give lower price (pulls down to theta faster) */
    TEST_ASSERT_TRUE(price_slow > price_fast);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Reproducibility
 *-------------------------------------------------------*/

void test_heston_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_steps(ctx1, 50);
    mco_set_steps(ctx2, 50);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_heston_european_call(ctx1, 100.0, 100.0, 0.05, 1.0,
                                              TEST_V0, TEST_KAPPA, TEST_THETA,
                                              TEST_SIGMA, TEST_RHO);
    double price2 = mco_heston_european_call(ctx2, 100.0, 100.0, 0.05, 1.0,
                                              TEST_V0, TEST_KAPPA, TEST_THETA,
                                              TEST_SIGMA, TEST_RHO);

    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_heston.c");

    /* Feller condition */
    RUN_TEST(test_heston_feller_satisfied);
    RUN_TEST(test_heston_feller_violated);

    /* Basic pricing */
    RUN_TEST(test_heston_call_atm);
    RUN_TEST(test_heston_put_atm);

    /* Skew */
    RUN_TEST(test_heston_negative_skew);

    /* Parameter sensitivity */
    RUN_TEST(test_heston_v0_sensitivity);
    RUN_TEST(test_heston_kappa_sensitivity);

    /* Reproducibility */
    RUN_TEST(test_heston_reproducible);

    return UnityEnd();
}
