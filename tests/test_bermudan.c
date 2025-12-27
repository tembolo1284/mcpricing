/*
 * Bermudan Option Tests
 *
 * Tests for Bermudan option pricing.
 * Verifies:
 *   - European ≤ Bermudan ≤ American
 *   - More exercise dates = higher price
 *   - Convergence to American as dates increase
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include <math.h>

#define BERMUDAN_TOLERANCE 0.60

/*-------------------------------------------------------
 * Basic Pricing Tests
 *-------------------------------------------------------*/

void test_bermudan_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    /* Quarterly exercise (4 opportunities) */
    double price = mco_bermudan_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 4);

    /* Should be between European (~$5.57) and American (~$6.08) */
    TEST_ASSERT_TRUE(price > 5.0);
    TEST_ASSERT_TRUE(price < 7.0);

    mco_ctx_free(ctx);
}

void test_bermudan_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    double price = mco_bermudan_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 4);

    /* Call should be close to European (early exercise not optimal) */
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 10.45, price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Value Ordering Tests
 *-------------------------------------------------------*/

void test_bermudan_between_european_american(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    double european = mco_european_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    mco_set_seed(ctx, 42);
    double bermudan = mco_bermudan_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 4);

    mco_set_seed(ctx, 42);
    double american = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 50);

    /* European ≤ Bermudan ≤ American (with MC noise tolerance) */
    TEST_ASSERT_TRUE(european <= bermudan + 0.3);
    TEST_ASSERT_TRUE(bermudan <= american + 0.3);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Exercise Frequency Tests
 *-------------------------------------------------------*/

void test_bermudan_more_exercise_dates(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    /* Monthly exercise (12 opportunities) */
    double price_12 = mco_bermudan_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    mco_set_seed(ctx, 42);
    /* Quarterly exercise (4 opportunities) */
    double price_4 = mco_bermudan_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 4);

    /* More exercise opportunities should give higher (or equal) price */
    TEST_ASSERT_TRUE(price_12 >= price_4 - 0.3);

    mco_ctx_free(ctx);
}

void test_bermudan_converges_to_american(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    /* Bermudan with many exercise dates should approach American */
    double bermudan_52 = mco_bermudan_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 52);

    mco_set_seed(ctx, 42);
    double american = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 52);

    /* Should be close */
    TEST_ASSERT_DOUBLE_WITHIN(0.50, american, bermudan_52);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * ITM/OTM Tests
 *-------------------------------------------------------*/

void test_bermudan_put_itm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    /* Deep ITM put: S=80, K=100 */
    double price = mco_bermudan_put(ctx, 80.0, 100.0, 0.05, 0.20, 1.0, 4);

    /* Should be at least intrinsic value = 20 */
    TEST_ASSERT_TRUE(price >= 19.0);
    TEST_ASSERT_TRUE(price < 25.0);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Reproducibility
 *-------------------------------------------------------*/

void test_bermudan_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_bermudan_put(ctx1, 100.0, 100.0, 0.05, 0.20, 1.0, 4);
    double price2 = mco_bermudan_put(ctx2, 100.0, 100.0, 0.05, 0.20, 1.0, 4);

    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_bermudan.c");

    RUN_TEST(test_bermudan_put_atm);
    RUN_TEST(test_bermudan_call_atm);
    RUN_TEST(test_bermudan_between_european_american);
    RUN_TEST(test_bermudan_more_exercise_dates);
    RUN_TEST(test_bermudan_converges_to_american);
    RUN_TEST(test_bermudan_put_itm);
    RUN_TEST(test_bermudan_reproducible);

    return UnityEnd();
}
