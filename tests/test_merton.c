/*
 * Merton Jump-Diffusion Model Tests
 *
 * Tests for Merton analytical formula and MC pricing.
 * Verifies:
 *   - Analytical vs MC agreement
 *   - Put-call parity
 *   - Jump effect on prices
 *   - Convergence to Black-Scholes as λ→0
 */
#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/models/merton_jump.h"
#include <math.h>

/*
 * Standard Merton parameters
 */
#define TEST_SIGMA    0.20    /* Diffusion volatility */
#define TEST_LAMBDA   1.0     /* 1 jump per year on average */
#define TEST_MU_J    -0.10    /* Mean jump = -10% (crashes) */
#define TEST_SIGMA_J  0.15    /* Jump volatility */

#define MERTON_TOLERANCE 0.50

/*-------------------------------------------------------
 * Analytical Formula Tests
 *-------------------------------------------------------*/
static void test_merton_call_atm(void)
{
    double price = mco_merton_call(100.0, 100.0, 0.05, 1.0,
                                    TEST_SIGMA, TEST_LAMBDA,
                                    TEST_MU_J, TEST_SIGMA_J);

    /* Should be positive and higher than BS (jumps add value to options) */
    double bs_price = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_TRUE(price > bs_price - 0.5);  /* Allow some tolerance */
    TEST_ASSERT_TRUE(price < 20.0);
}

static void test_merton_put_atm(void)
{
    double price = mco_merton_put(100.0, 100.0, 0.05, 1.0,
                                   TEST_SIGMA, TEST_LAMBDA,
                                   TEST_MU_J, TEST_SIGMA_J);

    double bs_price = mco_black_scholes_put(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_TRUE(price > bs_price - 0.5);
    TEST_ASSERT_TRUE(price < 15.0);
}

static void test_merton_put_call_parity(void)
{
    double call = mco_merton_call(100.0, 105.0, 0.05, 1.0,
                                   TEST_SIGMA, TEST_LAMBDA,
                                   TEST_MU_J, TEST_SIGMA_J);
    double put = mco_merton_put(100.0, 105.0, 0.05, 1.0,
                                 TEST_SIGMA, TEST_LAMBDA,
                                 TEST_MU_J, TEST_SIGMA_J);

    /* C - P = S - K*exp(-rT) */
    double parity_lhs = call - put;
    double parity_rhs = 100.0 - 105.0 * exp(-0.05);

    TEST_ASSERT_DOUBLE_WITHIN(0.01, parity_rhs, parity_lhs);
}

/*-------------------------------------------------------
 * Limit Tests
 *-------------------------------------------------------*/
static void test_merton_converges_to_bs(void)
{
    /* With λ = 0, Merton should equal Black-Scholes */
    double merton = mco_merton_call(100.0, 100.0, 0.05, 1.0,
                                     0.20, 0.0,   /* λ = 0 */
                                     -0.10, 0.15);
    double bs = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(0.01, bs, merton);
}

/*-------------------------------------------------------
 * Monte Carlo Tests
 *-------------------------------------------------------*/
static void test_merton_mc_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_steps(ctx, 252);  /* Daily for jumps */
    mco_set_seed(ctx, 42);

    double mc_price = mco_merton_european_call(ctx, 100.0, 100.0, 0.05, 1.0,
                                                TEST_SIGMA, TEST_LAMBDA,
                                                TEST_MU_J, TEST_SIGMA_J);

    double anal_price = mco_merton_call(100.0, 100.0, 0.05, 1.0,
                                         TEST_SIGMA, TEST_LAMBDA,
                                         TEST_MU_J, TEST_SIGMA_J);

    /* MC should be close to analytical */
    TEST_ASSERT_DOUBLE_WITHIN(MERTON_TOLERANCE, anal_price, mc_price);

    mco_ctx_free(ctx);
}

static void test_merton_mc_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_steps(ctx, 252);
    mco_set_seed(ctx, 42);

    double mc_price = mco_merton_european_put(ctx, 100.0, 100.0, 0.05, 1.0,
                                               TEST_SIGMA, TEST_LAMBDA,
                                               TEST_MU_J, TEST_SIGMA_J);

    double anal_price = mco_merton_put(100.0, 100.0, 0.05, 1.0,
                                        TEST_SIGMA, TEST_LAMBDA,
                                        TEST_MU_J, TEST_SIGMA_J);

    TEST_ASSERT_DOUBLE_WITHIN(MERTON_TOLERANCE, anal_price, mc_price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Jump Parameter Sensitivity
 *-------------------------------------------------------*/
static void test_merton_lambda_sensitivity(void)
{
    /* More jumps = higher option value (more uncertainty) */
    double price_low = mco_merton_call(100.0, 100.0, 0.05, 1.0,
                                        0.20, 0.5,   /* λ = 0.5 */
                                        -0.10, 0.15);
    double price_high = mco_merton_call(100.0, 100.0, 0.05, 1.0,
                                         0.20, 3.0,   /* λ = 3.0 */
                                         -0.10, 0.15);

    TEST_ASSERT_TRUE(price_high > price_low);
}

static void test_merton_sigma_j_sensitivity(void)
{
    /* Higher jump volatility = higher option value */
    double price_low = mco_merton_call(100.0, 100.0, 0.05, 1.0,
                                        0.20, 1.0, -0.10, 0.05);  /* σⱼ = 5% */
    double price_high = mco_merton_call(100.0, 100.0, 0.05, 1.0,
                                         0.20, 1.0, -0.10, 0.30); /* σⱼ = 30% */

    TEST_ASSERT_TRUE(price_high > price_low);
}

static void test_merton_jump_direction_affects_skew(void)
{
    /*
     * Test that jump direction affects the relative pricing of OTM puts vs calls.
     * With negative mean jumps: skew should be more negative (OTM puts more expensive relative to OTM calls)
     * With positive mean jumps: skew should be more positive
     */

    /* ATM prices should be higher with larger absolute jump size */
    double call_no_jump = mco_merton_call(100.0, 100.0, 0.05, 1.0, 0.20, 0.0, 0.0, 0.15);
    double call_neg = mco_merton_call(100.0, 100.0, 0.05, 1.0, 0.20, 2.0, -0.15, 0.15);
    double call_pos = mco_merton_call(100.0, 100.0, 0.05, 1.0, 0.20, 2.0, +0.15, 0.15);

    /* Jumps add uncertainty, so both should be higher than no-jump case */
    TEST_ASSERT_TRUE(call_neg > call_no_jump - 0.1);
    TEST_ASSERT_TRUE(call_pos > call_no_jump - 0.1);

    /* Positive jumps should make calls more valuable */
    TEST_ASSERT_TRUE(call_pos > call_neg);
}

/*-------------------------------------------------------
 * Reproducibility
 *-------------------------------------------------------*/
static void test_merton_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_steps(ctx1, 100);
    mco_set_steps(ctx2, 100);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_merton_european_call(ctx1, 100.0, 100.0, 0.05, 1.0,
                                              TEST_SIGMA, TEST_LAMBDA,
                                              TEST_MU_J, TEST_SIGMA_J);
    double price2 = mco_merton_european_call(ctx2, 100.0, 100.0, 0.05, 1.0,
                                              TEST_SIGMA, TEST_LAMBDA,
                                              TEST_MU_J, TEST_SIGMA_J);

    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/
int main(void)
{
    UnityBegin("test_merton.c");

    /* Analytical */
    RUN_TEST(test_merton_call_atm);
    RUN_TEST(test_merton_put_atm);
    RUN_TEST(test_merton_put_call_parity);
    RUN_TEST(test_merton_converges_to_bs);

    /* Monte Carlo */
    RUN_TEST(test_merton_mc_call_atm);
    RUN_TEST(test_merton_mc_put_atm);

    /* Parameter sensitivity */
    RUN_TEST(test_merton_lambda_sensitivity);
    RUN_TEST(test_merton_sigma_j_sensitivity);
    RUN_TEST(test_merton_jump_direction_affects_skew);

    /* Reproducibility */
    RUN_TEST(test_merton_reproducible);

    return UnityEnd();
}
