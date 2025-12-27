/*
 * Asian Option Tests
 *
 * Tests for Asian option pricing.
 * Verifies:
 *   - Arithmetic Asian < European (averaging reduces variance)
 *   - Geometric Asian matches closed-form
 *   - Convergence with observations
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/instruments/asian.h"
#include <math.h>

#define ASIAN_TOLERANCE 0.50

/*-------------------------------------------------------
 * Basic Pricing Tests
 *-------------------------------------------------------*/

void test_asian_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_asian_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    /* Asian call should be positive and less than European (~$10.45) */
    TEST_ASSERT_TRUE(price > 0.0);
    TEST_ASSERT_TRUE(price < 12.0);

    mco_ctx_free(ctx);
}

void test_asian_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_asian_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    /* Asian put should be positive and less than European (~$5.57) */
    TEST_ASSERT_TRUE(price > 0.0);
    TEST_ASSERT_TRUE(price < 8.0);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Asian vs European Tests
 *-------------------------------------------------------*/

void test_asian_less_than_european(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double asian = mco_asian_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    mco_set_seed(ctx, 42);
    double european = mco_european_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    /* Asian should be cheaper due to averaging */
    TEST_ASSERT_TRUE(asian < european + 0.5);  /* Allow MC noise */

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Geometric Asian Tests
 *-------------------------------------------------------*/

void test_asian_geometric_call(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double mc_price = mco_asian_geometric_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    /* Compare to closed-form */
    double closed = mco_asian_geometric_closed(100.0, 100.0, 0.05, 0.20, 1.0, 12, MCO_CALL);

    TEST_ASSERT_DOUBLE_WITHIN(ASIAN_TOLERANCE, closed, mc_price);

    mco_ctx_free(ctx);
}

void test_asian_geometric_put(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double mc_price = mco_asian_geometric_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    /* Compare to closed-form */
    double closed = mco_asian_geometric_closed(100.0, 100.0, 0.05, 0.20, 1.0, 12, MCO_PUT);

    TEST_ASSERT_DOUBLE_WITHIN(ASIAN_TOLERANCE, closed, mc_price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Observation Count Tests
 *-------------------------------------------------------*/

void test_asian_more_observations(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    /* More observations = smoother average = lower price */
    double price_12 = mco_asian_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    mco_set_seed(ctx, 42);
    double price_52 = mco_asian_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 52);

    /* Both should be reasonable */
    TEST_ASSERT_TRUE(price_12 > 0.0);
    TEST_ASSERT_TRUE(price_52 > 0.0);

    /* With enough observations, price should stabilize */
    TEST_ASSERT_DOUBLE_WITHIN(1.0, price_12, price_52);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Reproducibility
 *-------------------------------------------------------*/

void test_asian_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_asian_call(ctx1, 100.0, 100.0, 0.05, 0.20, 1.0, 12);
    double price2 = mco_asian_call(ctx2, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_asian.c");

    RUN_TEST(test_asian_call_atm);
    RUN_TEST(test_asian_put_atm);
    RUN_TEST(test_asian_less_than_european);
    RUN_TEST(test_asian_geometric_call);
    RUN_TEST(test_asian_geometric_put);
    RUN_TEST(test_asian_more_observations);
    RUN_TEST(test_asian_reproducible);

    return UnityEnd();
}
