/*
 * Lookback Options Tests
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/instruments/lookback.h"
#include <math.h>

#define LOOKBACK_TOL 1.0

void test_lookback_floating_call(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_lookback_call(ctx, 100.0, 0.0, 0.05, 0.20, 1.0, 252, 1);

    /* Floating call always has positive payoff (S(T) - min) */
    TEST_ASSERT_TRUE(price > 5.0);
    TEST_ASSERT_TRUE(price < 30.0);

    mco_ctx_free(ctx);
}

void test_lookback_floating_put(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_lookback_put(ctx, 100.0, 0.0, 0.05, 0.20, 1.0, 252, 1);

    /* Floating put always has positive payoff (max - S(T)) */
    TEST_ASSERT_TRUE(price > 5.0);
    TEST_ASSERT_TRUE(price < 30.0);

    mco_ctx_free(ctx);
}

void test_lookback_fixed_call(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_lookback_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 252, 0);

    /* Fixed call: max(max(S) - K, 0) - should be >= vanilla */
    double vanilla = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_TRUE(price >= vanilla - 1.0);

    mco_ctx_free(ctx);
}

void test_lookback_fixed_put(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_lookback_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 252, 0);

    /* Fixed put: max(K - min(S), 0) - should be >= vanilla */
    double vanilla = mco_black_scholes_put(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_TRUE(price >= vanilla - 1.0);

    mco_ctx_free(ctx);
}

void test_lookback_more_obs_higher_price(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 30000);

    /* More observations = more likely to capture extremum */
    mco_set_seed(ctx, 42);
    double price_12 = mco_lookback_call(ctx, 100.0, 0.0, 0.05, 0.20, 1.0, 12, 1);

    mco_set_seed(ctx, 42);
    double price_252 = mco_lookback_call(ctx, 100.0, 0.0, 0.05, 0.20, 1.0, 252, 1);

    /* Daily monitoring should give higher or equal price */
    TEST_ASSERT_TRUE(price_252 >= price_12 - 0.5);

    mco_ctx_free(ctx);
}

void test_lookback_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double p1 = mco_lookback_call(ctx1, 100.0, 0.0, 0.05, 0.20, 1.0, 100, 1);
    double p2 = mco_lookback_call(ctx2, 100.0, 0.0, 0.05, 0.20, 1.0, 100, 1);

    TEST_ASSERT_EQUAL_DOUBLE(p1, p2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

int main(void)
{
    UnityBegin("test_lookback.c");

    RUN_TEST(test_lookback_floating_call);
    RUN_TEST(test_lookback_floating_put);
    RUN_TEST(test_lookback_fixed_call);
    RUN_TEST(test_lookback_fixed_put);
    RUN_TEST(test_lookback_more_obs_higher_price);
    RUN_TEST(test_lookback_reproducible);

    return UnityEnd();
}
