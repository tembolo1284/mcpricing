/*
 * American Option Tests
 *
 * Tests for American option pricing using LSM.
 * Verifies:
 *   - American put > European put (early exercise premium)
 *   - American call ≈ European call (no early exercise for non-dividend)
 *   - Convergence with increasing simulations
 *   - Edge cases
 */
#include "unity/unity.h"
#include "mcoptions.h"
#include "internal/models/gbm.h"
#include <math.h>

/*
 * Reference values from binomial tree with 1000 steps:
 *   American Put (S=100, K=100, r=5%, σ=20%, T=1): ~$6.08
 *   European Put: ~$5.57
 *   Early exercise premium: ~$0.51
 *
 * LSM with sufficient paths should be within ~$0.30 of these.
 */
#define AMERICAN_PUT_REF 6.08
#define EUROPEAN_PUT_REF 5.57
#define AMERICAN_CALL_REF 10.45  /* Same as European (no early exercise) */

/* Tolerance for LSM (higher variance than closed-form methods) */
#define LSM_TOLERANCE 0.50

/*-------------------------------------------------------
 * Basic Pricing Tests
 *-------------------------------------------------------*/
static void test_american_put_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 50);
    TEST_ASSERT_DOUBLE_WITHIN(LSM_TOLERANCE, AMERICAN_PUT_REF, price);

    mco_ctx_free(ctx);
}

static void test_american_call_atm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double price = mco_american_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 50);
    /* American call ≈ European call for non-dividend stock */
    TEST_ASSERT_DOUBLE_WITHIN(LSM_TOLERANCE, AMERICAN_CALL_REF, price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Early Exercise Premium Tests
 *-------------------------------------------------------*/
static void test_american_put_exceeds_european(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double american = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 50);

    mco_set_seed(ctx, 42);
    double european = mco_european_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    /* American put should be worth more due to early exercise */
    TEST_ASSERT_TRUE(american >= european - 0.10);  /* Small tolerance for MC noise */

    mco_ctx_free(ctx);
}

static void test_american_call_approx_european(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    double american = mco_american_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 50);

    mco_set_seed(ctx, 42);
    double european = mco_european_call(ctx, 100.0, 100.0, 0.05, 0.20, 1.0);

    /* American call ≈ European call (early exercise not optimal) */
    TEST_ASSERT_DOUBLE_WITHIN(0.50, european, american);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * ITM/OTM Tests
 *-------------------------------------------------------*/
static void test_american_put_itm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    /* ITM put: S=90, K=100 */
    double price = mco_american_put(ctx, 90.0, 100.0, 0.05, 0.20, 1.0, 50);

    /* ITM American put has significant early exercise value */
    /* Should be at least intrinsic value = 10 */
    TEST_ASSERT_TRUE(price >= 10.0);
    TEST_ASSERT_TRUE(price < 20.0);  /* Sanity check */

    mco_ctx_free(ctx);
}

static void test_american_put_otm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    /* OTM put: S=110, K=100 */
    double price = mco_american_put(ctx, 110.0, 100.0, 0.05, 0.20, 1.0, 50);

    /* OTM put should be positive but small */
    TEST_ASSERT_TRUE(price > 0.0);
    TEST_ASSERT_TRUE(price < 10.0);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Deep ITM Put Test (Early Exercise Likely)
 *-------------------------------------------------------*/
static void test_american_put_deep_itm(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 50000);
    mco_set_seed(ctx, 42);

    /* Deep ITM put: S=50, K=100 (intrinsic = 50) */
    double american = mco_american_put(ctx, 50.0, 100.0, 0.05, 0.20, 1.0, 50);

    /* Deep ITM American put should be close to intrinsic value */
    /* Early exercise is almost certainly optimal */
    TEST_ASSERT_DOUBLE_WITHIN(5.0, 50.0, american);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Convergence Tests
 *-------------------------------------------------------*/
static void test_american_put_convergence(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_seed(ctx, 42);

    /* Price with increasing simulations */
    mco_set_simulations(ctx, 10000);
    double price_10k = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 50);

    mco_set_seed(ctx, 42);
    mco_set_simulations(ctx, 50000);
    double price_50k = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 50);

    /* Both should be in reasonable range */
    TEST_ASSERT_DOUBLE_WITHIN(1.0, AMERICAN_PUT_REF, price_10k);
    TEST_ASSERT_DOUBLE_WITHIN(0.5, AMERICAN_PUT_REF, price_50k);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Steps Sensitivity Test
 *-------------------------------------------------------*/
static void test_american_put_steps_sensitivity(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);

    mco_set_simulations(ctx, 30000);
    mco_set_seed(ctx, 42);

    /* Fewer steps = fewer exercise opportunities = lower price */
    double price_12 = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 12);

    mco_set_seed(ctx, 42);
    double price_52 = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 52);

    /* More exercise opportunities should give higher (or equal) price */
    /* Allow some MC noise */
    TEST_ASSERT_TRUE(price_52 >= price_12 - 0.30);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Reproducibility Tests
 *-------------------------------------------------------*/
static void test_american_reproducible(void)
{
    mco_ctx *ctx1 = mco_ctx_new();
    mco_ctx *ctx2 = mco_ctx_new();

    mco_set_simulations(ctx1, 10000);
    mco_set_simulations(ctx2, 10000);
    mco_set_seed(ctx1, 12345);
    mco_set_seed(ctx2, 12345);

    double price1 = mco_american_put(ctx1, 100.0, 100.0, 0.05, 0.20, 1.0, 50);
    double price2 = mco_american_put(ctx2, 100.0, 100.0, 0.05, 0.20, 1.0, 50);

    /* Same seed = same result */
    TEST_ASSERT_EQUAL_DOUBLE(price1, price2);

    mco_ctx_free(ctx1);
    mco_ctx_free(ctx2);
}

/*-------------------------------------------------------
 * Edge Cases
 *-------------------------------------------------------*/
static void test_american_zero_time(void)
{
    mco_ctx *ctx = mco_ctx_new();

    /* Zero time = immediate exercise */
    double put = mco_american_put(ctx, 90.0, 100.0, 0.05, 0.20, 0.0, 50);

    /* Should equal intrinsic value */
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 10.0, put);

    mco_ctx_free(ctx);
}

static void test_american_default_steps(void)
{
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 20000);
    mco_set_seed(ctx, 42);

    /* num_steps = 0 should use default (52) */
    double price = mco_american_put(ctx, 100.0, 100.0, 0.05, 0.20, 1.0, 0);

    /* Should still produce reasonable result */
    TEST_ASSERT_DOUBLE_WITHIN(1.0, AMERICAN_PUT_REF, price);

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/
int main(void)
{
    UnityBegin("test_american.c");

    /* Basic pricing */
    RUN_TEST(test_american_put_atm);
    RUN_TEST(test_american_call_atm);

    /* Early exercise premium */
    RUN_TEST(test_american_put_exceeds_european);
    RUN_TEST(test_american_call_approx_european);

    /* ITM/OTM */
    RUN_TEST(test_american_put_itm);
    RUN_TEST(test_american_put_otm);
    RUN_TEST(test_american_put_deep_itm);

    /* Convergence */
    RUN_TEST(test_american_put_convergence);
    RUN_TEST(test_american_put_steps_sensitivity);

    /* Reproducibility */
    RUN_TEST(test_american_reproducible);

    /* Edge cases */
    RUN_TEST(test_american_zero_time);
    RUN_TEST(test_american_default_steps);

    return UnityEnd();
}
