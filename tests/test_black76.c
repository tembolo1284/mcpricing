/*
 * Black-76 Model Tests
 *
 * Tests for futures/forward options pricing.
 * Verifies:
 *   - Analytical formulas vs known values
 *   - Put-call parity for forwards
 *   - Greeks
 *   - Implied volatility solver
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/models/black76.h"
#include <math.h>

#define B76_TOLERANCE 0.001

/*
 * Reference values computed externally:
 * F=100, K=100, r=5%, σ=20%, T=1
 * Call ≈ 7.5771, Put ≈ 7.5771 (equal for ATM forward)
 */
#define B76_ATM_PRICE 7.5771

/*-------------------------------------------------------
 * Basic Pricing Tests
 *-------------------------------------------------------*/

void test_black76_call_atm(void)
{
    double price = mco_black76_call(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, B76_ATM_PRICE, price);
}

void test_black76_put_atm(void)
{
    double price = mco_black76_put(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, B76_ATM_PRICE, price);
}

void test_black76_call_itm(void)
{
    /* ITM call: F=100, K=90 */
    double price = mco_black76_call(100.0, 90.0, 0.05, 0.20, 1.0);
    
    /* Should be > ATM and > intrinsic */
    double intrinsic = exp(-0.05) * (100.0 - 90.0);
    TEST_ASSERT_TRUE(price > intrinsic);
    TEST_ASSERT_TRUE(price > B76_ATM_PRICE);
}

void test_black76_put_itm(void)
{
    /* ITM put: F=100, K=110 */
    double price = mco_black76_put(100.0, 110.0, 0.05, 0.20, 1.0);
    
    /* Should be > intrinsic */
    double intrinsic = exp(-0.05) * (110.0 - 100.0);
    TEST_ASSERT_TRUE(price > intrinsic);
}

/*-------------------------------------------------------
 * Put-Call Parity Tests
 *-------------------------------------------------------*/

void test_black76_put_call_parity(void)
{
    double F = 100.0, K = 105.0, r = 0.05, sigma = 0.25, T = 1.0;
    
    double call = mco_black76_call(F, K, r, sigma, T);
    double put = mco_black76_put(F, K, r, sigma, T);
    
    /* Put-call parity: C - P = e^(-rT)·(F - K) */
    double parity_lhs = call - put;
    double parity_rhs = exp(-r * T) * (F - K);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.001, parity_rhs, parity_lhs);
}

void test_black76_atm_symmetry(void)
{
    /* ATM: Call = Put for forward options */
    double call = mco_black76_call(100.0, 100.0, 0.05, 0.20, 1.0);
    double put = mco_black76_put(100.0, 100.0, 0.05, 0.20, 1.0);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.001, call, put);
}

/*-------------------------------------------------------
 * Greeks Tests
 *-------------------------------------------------------*/

void test_black76_delta_call(void)
{
    double delta = mco_black76_delta(100.0, 100.0, 0.05, 0.20, 1.0, 1);
    
    /* ATM call delta ≈ 0.5 * discount */
    double expected = 0.5 * exp(-0.05);
    TEST_ASSERT_DOUBLE_WITHIN(0.05, expected, delta);
}

void test_black76_delta_put(void)
{
    double delta = mco_black76_delta(100.0, 100.0, 0.05, 0.20, 1.0, 0);
    
    /* ATM put delta ≈ -0.5 * discount */
    double expected = -0.5 * exp(-0.05);
    TEST_ASSERT_DOUBLE_WITHIN(0.05, expected, delta);
}

void test_black76_gamma(void)
{
    double gamma = mco_black76_gamma(100.0, 100.0, 0.05, 0.20, 1.0);
    
    /* Gamma should be positive and significant for ATM */
    TEST_ASSERT_TRUE(gamma > 0.01);
    TEST_ASSERT_TRUE(gamma < 0.05);
}

void test_black76_vega(void)
{
    double vega = mco_black76_vega(100.0, 100.0, 0.05, 0.20, 1.0);
    
    /* Vega should be positive and significant */
    TEST_ASSERT_TRUE(vega > 30.0);
    TEST_ASSERT_TRUE(vega < 50.0);
}

void test_black76_theta(void)
{
    double theta_call = mco_black76_theta(100.0, 100.0, 0.05, 0.20, 1.0, 1);
    double theta_put = mco_black76_theta(100.0, 100.0, 0.05, 0.20, 1.0, 0);
    
    /* Time decay - theta typically negative for long options */
    /* But for Black-76 the sign depends on rates */
    TEST_ASSERT_TRUE(fabs(theta_call) < 20.0);
    TEST_ASSERT_TRUE(fabs(theta_put) < 20.0);
}

/*-------------------------------------------------------
 * Implied Volatility Tests
 *-------------------------------------------------------*/

void test_black76_implied_vol_call(void)
{
    double F = 100.0, K = 100.0, r = 0.05, sigma = 0.20, T = 1.0;
    
    /* Compute price, then recover vol */
    double price = mco_black76_call(F, K, r, sigma, T);
    double implied = mco_black76_implied_vol(F, K, r, T, price, 1);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.001, sigma, implied);
}

void test_black76_implied_vol_put(void)
{
    double F = 100.0, K = 110.0, r = 0.05, sigma = 0.25, T = 0.5;
    
    double price = mco_black76_put(F, K, r, sigma, T);
    double implied = mco_black76_implied_vol(F, K, r, T, price, 0);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.001, sigma, implied);
}

void test_black76_implied_vol_otm(void)
{
    double F = 100.0, K = 120.0, r = 0.05, sigma = 0.30, T = 1.0;
    
    double price = mco_black76_call(F, K, r, sigma, T);
    double implied = mco_black76_implied_vol(F, K, r, T, price, 1);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.002, sigma, implied);
}

/*-------------------------------------------------------
 * Edge Cases
 *-------------------------------------------------------*/

void test_black76_zero_time(void)
{
    /* At expiry: intrinsic value */
    double call = mco_black76_call(100.0, 90.0, 0.05, 0.20, 0.0);
    double put = mco_black76_put(100.0, 110.0, 0.05, 0.20, 0.0);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 10.0, call);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 10.0, put);
}

void test_black76_zero_vol(void)
{
    /* Zero vol: discounted intrinsic */
    double call = mco_black76_call(100.0, 90.0, 0.05, 0.0, 1.0);
    double expected = exp(-0.05) * (100.0 - 90.0);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.001, expected, call);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_black76.c");

    /* Basic pricing */
    RUN_TEST(test_black76_call_atm);
    RUN_TEST(test_black76_put_atm);
    RUN_TEST(test_black76_call_itm);
    RUN_TEST(test_black76_put_itm);

    /* Put-call parity */
    RUN_TEST(test_black76_put_call_parity);
    RUN_TEST(test_black76_atm_symmetry);

    /* Greeks */
    RUN_TEST(test_black76_delta_call);
    RUN_TEST(test_black76_delta_put);
    RUN_TEST(test_black76_gamma);
    RUN_TEST(test_black76_vega);
    RUN_TEST(test_black76_theta);

    /* Implied vol */
    RUN_TEST(test_black76_implied_vol_call);
    RUN_TEST(test_black76_implied_vol_put);
    RUN_TEST(test_black76_implied_vol_otm);

    /* Edge cases */
    RUN_TEST(test_black76_zero_time);
    RUN_TEST(test_black76_zero_vol);

    return UnityEnd();
}
