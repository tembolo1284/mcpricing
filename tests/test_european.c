/*
 * European Option Tests
 *
 * Tests for European call and put pricing.
 * Verifies:
 *   - MC prices converge to Black-Scholes analytical values
 *   - Antithetic variates reduce variance
 *   - Multi-threading produces correct results
 *   - Put-call parity holds
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/models/gbm.h"
#include <math.h>

/* Test tolerance - MC has inherent variance */
#define MC_TOLERANCE 1.00  /* $1.00 for 100K sims without variance reduction */
#define MC_TOLERANCE_TIGHT 0.30  /* Tighter for antithetic */

/*-------------------------------------------------------
 * Helper: Black-Scholes reference prices
 *-------------------------------------------------------*/

/* ATM call: S=100, K=100, r=5%, σ=20%, T=1 → BS ≈ $10.45 */
#define ATM_CALL_BS 10.4506

/* ATM put: via put-call parity → BS ≈ $5.57 */
#define ATM_PUT_BS 5.5735

/* ITM call: S=100, K=90 → BS ≈ $16.70 */
#define ITM_CALL_BS 16.6994

/* OTM call: S=100, K=110 → BS ≈ $6.04 */
#define OTM_CALL_BS 6.0401

/*-------------------------------------------------------
 * Basic Pricing Tests
 *-------------------------------------------------------*/

void test_european_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);

    double price = mco_european_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE, ATM_CALL_BS, price);

    mco_ctx_free(ctx);
}

void test_european_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);

    double price = mco_european_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE, ATM_PUT_BS, price);

    mco_ctx_free(ctx);
}

void test_european_call_itm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);

    double price = mco_european_call(ctx, 100.0, 90.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE, ITM_CALL_BS, price);

    mco_ctx_free(ctx);
}

void test_european_call_otm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);

    double price = mco_european_call(ctx, 100.0, 110.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE, OTM_CALL_BS, price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Antithetic Variates Tests
 *-------------------------------------------------------*/

void test_european_call_antithetic(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);
    mco_set_antithetic(ctx, 1);

    double price = mco_european_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    /* Antithetic should give tighter convergence */
    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE_TIGHT, ATM_CALL_BS, price);

    mco_ctx_free(ctx);
}

void test_european_put_antithetic(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);
    mco_set_antithetic(ctx, 1);

    double price = mco_european_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE_TIGHT, ATM_PUT_BS, price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Multi-Threading Tests
 *-------------------------------------------------------*/

void test_european_call_multithreaded(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);
    mco_set_threads(ctx, 4);

    double price = mco_european_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE, ATM_CALL_BS, price);

    mco_ctx_free(ctx);
}

void test_european_call_multithreaded_antithetic(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);
    mco_set_threads(ctx, 4);
    mco_set_antithetic(ctx, 1);

    double price = mco_european_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(MC_TOLERANCE_TIGHT, ATM_CALL_BS, price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Reproducibility Tests
 *-------------------------------------------------------*/

void test_european_reproducible_single_thread(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_european_call(ctx1, 100.0, 100.0, 0.05, 0.20, 1.0);
    double price2 = mco_european_call(ctx2, 100.0, 100.0, 0.05, 0.20, 1.0);

    /* Same seed = exact same result */
    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

void test_european_reproducible_multithreaded(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);
    mco_set_threads(ctx1, 4);
    mco_set_threads(ctx2, 4);

    double price1 = mco_european_call(ctx1, 100.0, 100.0, 0.05, 0.20, 1.0);
    double price2 = mco_european_call(ctx2, 100.0, 100.0, 0.05, 0.20, 1.0);

    /* Same seed + same thread count = exact same result */
    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Put-Call Parity Tests
 *-------------------------------------------------------*/

void test_put_call_parity(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 200000);
    mco_set_seed(ctx, 42);
    mco_set_antithetic(ctx, 1);

    double S = 100.0;
    double K = 100.0;
    double r = 0.05;
    double T = 1.0;

    double call = mco_european_call(ctx, S, K, r, 0.20, T);
    
    /* Reset RNG for put */
    mco_set_seed(ctx, 42);
    double put = mco_european_put(ctx, S, K, r, 0.20, T);

    /* Put-Call Parity: C - P = S - K*e^(-rT) */
    double parity_lhs = call - put;
    double parity_rhs = S - K * exp(-r * T);

    TEST_ASSERT_DOUBLE_WITHIN(0.30, parity_rhs, parity_lhs);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Black-Scholes Analytical Tests
 *-------------------------------------------------------*/

void test_black_scholes_call(void)
{
    double price = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, ATM_CALL_BS, price);
}

void test_black_scholes_put(void)
{
    double price = mco_black_scholes_put(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, ATM_PUT_BS, price);
}

/*-------------------------------------------------------
 * Edge Cases
 *-------------------------------------------------------*/

void test_european_zero_volatility(void)
{
    mco_ctx *ctx = mco_ctx_new();
    
    /* Zero vol call: max(S - K*e^(-rT), 0) */
    double price = mco_european_call(ctx, 100.0, 90.0, 0.05, 0.0, 1.0);
    double expected = 100.0 - 90.0 * exp(-0.05);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.01, expected, price);
    
    mco_ctx_free(ctx);
}

void test_european_zero_time(void)
{
    mco_ctx *ctx = mco_ctx_new();
    
    /* Zero time = immediate exercise */
    double call = mco_european_call(ctx, 100.0, 90.0, 0.05, 0.20, 0.0);
    double put = mco_european_put(ctx, 100.0, 110.0, 0.05, 0.20, 0.0);
    
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 10.0, call);  /* max(100-90, 0) */
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 10.0, put);   /* max(110-100, 0) */
    
    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_european.c");

    /* Basic pricing */
    RUN_TEST(test_european_call_atm);
    RUN_TEST(test_european_put_atm);
    RUN_TEST(test_european_call_itm);
    RUN_TEST(test_european_call_otm);

    /* Antithetic variates */
    RUN_TEST(test_european_call_antithetic);
    RUN_TEST(test_european_put_antithetic);

    /* Multi-threading */
    RUN_TEST(test_european_call_multithreaded);
    RUN_TEST(test_european_call_multithreaded_antithetic);

    /* Reproducibility */
    RUN_TEST(test_european_reproducible_single_thread);
    RUN_TEST(test_european_reproducible_multithreaded);

    /* Put-call parity */
    RUN_TEST(test_put_call_parity);

    /* Black-Scholes analytical */
    RUN_TEST(test_black_scholes_call);
    RUN_TEST(test_black_scholes_put);

    /* Edge cases */
    RUN_TEST(test_european_zero_volatility);
    RUN_TEST(test_european_zero_time);

    return UnityEnd();
}
