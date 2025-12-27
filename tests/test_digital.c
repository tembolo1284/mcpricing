/*
 * Digital (Binary) Options Tests
 */

#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/instruments/digital.h"
#include <math.h>

#define DIGITAL_TOL 0.30

void test_digital_cash_call_atm(void)
{
    /* ATM digital: ~50% chance of payout */
    double price = mco_digital_cash_call(100.0, 100.0, 1.0, 0.05, 0.20, 1.0);

    /* Should be around 0.5 * exp(-rT) = 0.5 * 0.951 ≈ 0.476 */
    TEST_ASSERT_DOUBLE_WITHIN(0.10, 0.476, price);
}

void test_digital_cash_put_atm(void)
{
    double price = mco_digital_cash_put(100.0, 100.0, 1.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_DOUBLE_WITHIN(0.10, 0.476, price);
}

void test_digital_cash_parity(void)
{
    /* Cash call + Cash put = payout * discount */
    double call = mco_digital_cash_call(100.0, 100.0, 1.0, 0.05, 0.20, 1.0);
    double put = mco_digital_cash_put(100.0, 100.0, 1.0, 0.05, 0.20, 1.0);

    double parity = 1.0 * exp(-0.05);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, parity, call + put);
}

void test_digital_asset_call_atm(void)
{
    double price = mco_digital_asset_call(100.0, 100.0, 0.05, 0.20, 1.0);

    /* Asset-or-nothing: S * N(d1) */
    TEST_ASSERT_TRUE(price > 40.0);
    TEST_ASSERT_TRUE(price < 70.0);
}

void test_digital_asset_put_atm(void)
{
    double price = mco_digital_asset_put(100.0, 100.0, 0.05, 0.20, 1.0);
    TEST_ASSERT_TRUE(price > 30.0);
    TEST_ASSERT_TRUE(price < 60.0);
}

void test_digital_mc_vs_analytical(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 100000);
    mco_set_seed(ctx, 42);

    double mc = mco_digital_call(ctx, 100.0, 100.0, 1.0, 0.05, 0.20, 1.0, 1);
    double anal = mco_digital_cash_call(100.0, 100.0, 1.0, 0.05, 0.20, 1.0);

    TEST_ASSERT_DOUBLE_WITHIN(0.02, anal, mc);

    mco_ctx_free(ctx);
}

void test_digital_itm(void)
{
    /* Deep ITM: should be significantly higher than ATM */
    double itm = mco_digital_cash_call(120.0, 100.0, 1.0, 0.05, 0.20, 1.0);
    double atm = mco_digital_cash_call(100.0, 100.0, 1.0, 0.05, 0.20, 1.0);
    
    TEST_ASSERT_TRUE(itm > atm);
    TEST_ASSERT_TRUE(itm > 0.7);
}

void test_digital_otm(void)
{
    /* Deep OTM: should be significantly lower than ATM */
    double otm = mco_digital_cash_call(80.0, 100.0, 1.0, 0.05, 0.20, 1.0);
    double atm = mco_digital_cash_call(100.0, 100.0, 1.0, 0.05, 0.20, 1.0);
    
    TEST_ASSERT_TRUE(otm < atm);
    TEST_ASSERT_TRUE(otm < 0.25);
}

void test_digital_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double p1 = mco_digital_call(ctx1, 100.0, 100.0, 1.0, 0.05, 0.20, 1.0, 1);
    double p2 = mco_digital_call(ctx2, 100.0, 100.0, 1.0, 0.05, 0.20, 1.0, 1);

    TEST_ASSERT_EQUAL_DOUBLE(p1, p2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

int main(void)
{
    UnityBegin("test_digital.c");

    RUN_TEST(test_digital_cash_call_atm);
    RUN_TEST(test_digital_cash_put_atm);
    RUN_TEST(test_digital_cash_parity);
    RUN_TEST(test_digital_asset_call_atm);
    RUN_TEST(test_digital_asset_put_atm);
    RUN_TEST(test_digital_mc_vs_analytical);
    RUN_TEST(test_digital_itm);
    RUN_TEST(test_digital_otm);
    RUN_TEST(test_digital_reproducible);

    return UnityEnd();
}
