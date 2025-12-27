/*
 * SABR Model Tests
 *
 * Tests for SABR stochastic volatility model.
 * Verifies:
 *   - Hagan implied vol formula
 *   - MC pricing vs Hagan approximation
 *   - Volatility smile shape
 *   - Parameter sensitivity
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/models/sabr.h"
#include "internal/models/gbm.h"
#include <math.h>

#define SABR_TOLERANCE 0.50

/*
 * Standard SABR parameters for testing
 * Using beta=1 (lognormal) for easier validation against Black-Scholes
 */
#define TEST_ALPHA 0.20   /* Initial vol (equals ATM Black vol when beta=1) */
#define TEST_BETA  1.00   /* Lognormal SABR */
#define TEST_RHO  -0.25   /* Negative skew (typical) */
#define TEST_NU    0.40   /* Vol of vol */

/*-------------------------------------------------------
 * Hagan Implied Vol Tests
 *-------------------------------------------------------*/

void test_sabr_atm_vol(void)
{
    /* ATM implied vol should be close to alpha for short expiry */
    double atm_vol = mco_sabr_atm_vol(100.0, 0.25, TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);

    /* Should be in reasonable range */
    TEST_ASSERT_TRUE(atm_vol > 0.15);
    TEST_ASSERT_TRUE(atm_vol < 0.30);
}

void test_sabr_implied_vol_smile(void)
{
    double forward = 100.0;
    double time = 1.0;

    /* Calculate implied vols at different strikes */
    double vol_80  = mco_sabr_implied_vol(forward, 80.0, time, TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);
    double vol_100 = mco_sabr_implied_vol(forward, 100.0, time, TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);
    double vol_120 = mco_sabr_implied_vol(forward, 120.0, time, TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);

    /* All should be positive */
    TEST_ASSERT_TRUE(vol_80 > 0.0);
    TEST_ASSERT_TRUE(vol_100 > 0.0);
    TEST_ASSERT_TRUE(vol_120 > 0.0);

    /* With negative rho, we expect negative skew (puts have higher vol) */
    TEST_ASSERT_TRUE(vol_80 > vol_100);  /* Downside vol higher */
}

void test_sabr_implied_vol_symmetry(void)
{
    /* With rho = 0 and beta = 1, smile should be roughly symmetric */
    double forward = 100.0;
    double time = 1.0;
    double alpha = 0.20;
    double beta = 1.0;
    double rho = 0.0;
    double nu = 0.40;

    double vol_90  = mco_sabr_implied_vol(forward, 90.0, time, alpha, beta, rho, nu);
    double vol_110 = mco_sabr_implied_vol(forward, 110.0, time, alpha, beta, rho, nu);

    /* Should be close but not exact due to lognormal dynamics */
    TEST_ASSERT_DOUBLE_WITHIN(0.02, vol_90, vol_110);
}

/*-------------------------------------------------------
 * SABR MC Pricing Tests
 *-------------------------------------------------------*/

void test_sabr_european_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_steps(ctx, 100);
    mco_set_seed(ctx, 42);

    double price = mco_sabr_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                           TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);

    /* Should be positive and in reasonable range */
    TEST_ASSERT_TRUE(price > 5.0);
    TEST_ASSERT_TRUE(price < 20.0);

    mco_ctx_free(ctx);
}

void test_sabr_european_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_steps(ctx, 100);
    mco_set_seed(ctx, 42);

    double price = mco_sabr_european_put(ctx, 100.0, 100.0, 0.05, 1.0,
                                          TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);

    TEST_ASSERT_TRUE(price > 2.0);
    TEST_ASSERT_TRUE(price < 15.0);

    mco_ctx_free(ctx);
}

void test_sabr_mc_vs_hagan(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_steps(ctx, 200);
    mco_set_seed(ctx, 42);

    double forward = 100.0;
    double strike = 100.0;
    double rate = 0.05;
    double time = 1.0;

    /* Get SABR implied vol */
    double sabr_vol = mco_sabr_implied_vol(forward, strike, time, 
                                            TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);

    /* Price with Black-Scholes using SABR implied vol */
    double bs_price = mco_black_scholes_call(forward, strike, rate, sabr_vol, time);

    /* Price with SABR MC */
    double mc_price = mco_sabr_european_call(ctx, forward, strike, rate, time,
                                              TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);

    /* Should be reasonably close (Hagan is an approximation, especially for high vol-of-vol) */
    TEST_ASSERT_DOUBLE_WITHIN(3.0, bs_price, mc_price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Parameter Sensitivity Tests
 *-------------------------------------------------------*/

void test_sabr_alpha_sensitivity(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 30000);
    mco_set_steps(ctx, 100);

    /* Higher alpha = higher option prices */
    mco_set_seed(ctx, 42);
    double price_low = mco_sabr_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                               0.15, TEST_BETA, TEST_RHO, TEST_NU);

    mco_set_seed(ctx, 42);
    double price_high = mco_sabr_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                                0.25, TEST_BETA, TEST_RHO, TEST_NU);

    TEST_ASSERT_TRUE(price_high > price_low);

    mco_ctx_free(ctx);
}

void test_sabr_nu_sensitivity(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 30000);
    mco_set_steps(ctx, 100);

    /* Higher nu (vol of vol) = higher OTM option prices */
    mco_set_seed(ctx, 42);
    double price_low = mco_sabr_european_call(ctx, 100.0, 120.0, 0.05, 1.0,
                                               TEST_ALPHA, TEST_BETA, TEST_RHO, 0.20);

    mco_set_seed(ctx, 42);
    double price_high = mco_sabr_european_call(ctx, 100.0, 120.0, 0.05, 1.0,
                                                TEST_ALPHA, TEST_BETA, TEST_RHO, 0.60);

    TEST_ASSERT_TRUE(price_high > price_low);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Reproducibility
 *-------------------------------------------------------*/

void test_sabr_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_steps(ctx1, 50);
    mco_set_steps(ctx2, 50);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_sabr_european_call(ctx1, 100.0, 100.0, 0.05, 1.0,
                                            TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);
    double price2 = mco_sabr_european_call(ctx2, 100.0, 100.0, 0.05, 1.0,
                                            TEST_ALPHA, TEST_BETA, TEST_RHO, TEST_NU);

    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_sabr.c");

    /* Hagan formula */
    RUN_TEST(test_sabr_atm_vol);
    RUN_TEST(test_sabr_implied_vol_smile);
    RUN_TEST(test_sabr_implied_vol_symmetry);

    /* MC pricing */
    RUN_TEST(test_sabr_european_call_atm);
    RUN_TEST(test_sabr_european_put_atm);
    RUN_TEST(test_sabr_mc_vs_hagan);

    /* Parameter sensitivity */
    RUN_TEST(test_sabr_alpha_sensitivity);
    RUN_TEST(test_sabr_nu_sensitivity);

    /* Reproducibility */
    RUN_TEST(test_sabr_reproducible);

    return UnityEnd();
}
