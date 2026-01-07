/*
 * Barrier Options Tests
 */
#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/instruments/barrier.h"
#include <math.h>

#define BARRIER_TOL 1.0

static void test_barrier_down_out_call_no_hit(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    /* Barrier far below spot - unlikely to hit */
    double price = mco_barrier_call(ctx, 100.0, 100.0, 60.0, 0.05, 0.20, 1.0, 252,
                                     MCO_BARRIER_DOWN_OUT);

    /* Should be close to vanilla call */
    double vanilla = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(2.0, vanilla, price);

    mco_ctx_free(ctx);
}

static void test_barrier_down_out_call_near_barrier(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    /* Barrier close to spot - higher chance of knock-out */
    double price = mco_barrier_call(ctx, 100.0, 100.0, 90.0, 0.05, 0.20, 1.0, 252,
                                     MCO_BARRIER_DOWN_OUT);

    /* Should be less than vanilla due to knock-out risk */
    double vanilla = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_TRUE(price < vanilla);
    TEST_ASSERT_TRUE(price > 0.0);

    mco_ctx_free(ctx);
}

static void test_barrier_up_out_call(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_barrier_call(ctx, 100.0, 100.0, 120.0, 0.05, 0.20, 1.0, 252,
                                     MCO_BARRIER_UP_OUT);

    /* Up-and-out call: knocked out if price rises too much */
    TEST_ASSERT_TRUE(price > 0.0);
    TEST_ASSERT_TRUE(price < 15.0);

    mco_ctx_free(ctx);
}

static void test_barrier_knock_in_out_parity(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 50000);

    /* Knock-in + Knock-out = Vanilla (approximately) */
    mco_set_seed(ctx, 42);
    double down_out = mco_barrier_call(ctx, 100.0, 100.0, 80.0, 0.05, 0.20, 1.0, 252,
                                        MCO_BARRIER_DOWN_OUT);

    mco_set_seed(ctx, 42);
    double down_in = mco_barrier_call(ctx, 100.0, 100.0, 80.0, 0.05, 0.20, 1.0, 252,
                                       MCO_BARRIER_DOWN_IN);

    double vanilla = mco_black_scholes_call(100.0, 100.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(1.0, vanilla, down_out + down_in);

    mco_ctx_free(ctx);
}

static void test_barrier_analytical_vs_mc(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);

    double mc_price = mco_barrier_call(ctx, 100.0, 100.0, 80.0, 0.05, 0.20, 1.0, 252,
                                        MCO_BARRIER_DOWN_OUT);

    double anal_price = mco_barrier_down_out_call(100.0, 100.0, 80.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(1.0, anal_price, mc_price);

    mco_ctx_free(ctx);
}

static void test_barrier_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double p1 = mco_barrier_call(ctx1, 100.0, 100.0, 80.0, 0.05, 0.20, 1.0, 100,
                                  MCO_BARRIER_DOWN_OUT);
    double p2 = mco_barrier_call(ctx2, 100.0, 100.0, 80.0, 0.05, 0.20, 1.0, 100,
                                  MCO_BARRIER_DOWN_OUT);

    TEST_ASSERT_EQUAL_DOUBLE(p1, p2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

int main(void)
{
    UnityBegin("test_barrier.c");

    RUN_TEST(test_barrier_down_out_call_no_hit);
    RUN_TEST(test_barrier_down_out_call_near_barrier);
    RUN_TEST(test_barrier_up_out_call);
    RUN_TEST(test_barrier_knock_in_out_parity);
    RUN_TEST(test_barrier_analytical_vs_mc);
    RUN_TEST(test_barrier_reproducible);

    return UnityEnd();
}
