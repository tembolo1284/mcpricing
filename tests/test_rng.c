/*
 * RNG Tests
 *
 * Tests for the Xoshiro256** random number generator.
 * Verifies:
 *   - Seeding produces deterministic sequences
 *   - Uniform distribution in [0, 1)
 *   - Normal distribution has correct mean/variance
 *   - Jump produces independent streams
 */
#include "unity/unity.h"
#include "internal/rng.h"
#include <math.h>

/*-------------------------------------------------------
 * Determinism Tests
 *-------------------------------------------------------*/
static void test_rng_deterministic_seed(void)
{
    mco_rng rng1, rng2;
    mco_rng_seed(&rng1, 12345);
    mco_rng_seed(&rng2, 12345);

    /* Same seed should produce same sequence */
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_EQUAL_HEX64(mco_rng_next(&rng1), mco_rng_next(&rng2));
    }
}

static void test_rng_different_seeds(void)
{
    mco_rng rng1, rng2;
    mco_rng_seed(&rng1, 12345);
    mco_rng_seed(&rng2, 54321);

    /* Different seeds should produce different sequences */
    int same_count = 0;
    for (int i = 0; i < 100; i++) {
        if (mco_rng_next(&rng1) == mco_rng_next(&rng2)) {
            same_count++;
        }
    }
    /* Extremely unlikely to have any matches */
    TEST_ASSERT_EQUAL_INT(0, same_count);
}

/*-------------------------------------------------------
 * Uniform Distribution Tests
 *-------------------------------------------------------*/
static void test_rng_uniform_range(void)
{
    mco_rng rng;
    mco_rng_seed(&rng, 42);

    /* All values should be in [0, 1) */
    for (int i = 0; i < 10000; i++) {
        double u = mco_rng_uniform(&rng);
        TEST_ASSERT_TRUE(u >= 0.0);
        TEST_ASSERT_TRUE(u < 1.0);
    }
}

static void test_rng_uniform_mean(void)
{
    mco_rng rng;
    mco_rng_seed(&rng, 42);

    /* Mean should be ~0.5 */
    double sum = 0.0;
    int n = 100000;
    for (int i = 0; i < n; i++) {
        sum += mco_rng_uniform(&rng);
    }
    double mean = sum / n;
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.5, mean);
}

/*-------------------------------------------------------
 * Normal Distribution Tests
 *-------------------------------------------------------*/
static void test_rng_normal_mean(void)
{
    mco_rng rng;
    mco_rng_seed(&rng, 42);

    /* Mean should be ~0 */
    double sum = 0.0;
    int n = 100000;
    for (int i = 0; i < n; i++) {
        sum += mco_rng_normal(&rng);
    }
    double mean = sum / n;
    TEST_ASSERT_DOUBLE_WITHIN(0.02, 0.0, mean);
}

static void test_rng_normal_variance(void)
{
    mco_rng rng;
    mco_rng_seed(&rng, 42);

    /* Variance should be ~1 */
    double sum = 0.0;
    double sum_sq = 0.0;
    int n = 100000;
    for (int i = 0; i < n; i++) {
        double z = mco_rng_normal(&rng);
        sum += z;
        sum_sq += z * z;
    }
    double mean = sum / n;
    double variance = (sum_sq / n) - (mean * mean);
    TEST_ASSERT_DOUBLE_WITHIN(0.02, 1.0, variance);
}

/*-------------------------------------------------------
 * Jump Tests
 *-------------------------------------------------------*/
static void test_rng_jump_different_streams(void)
{
    mco_rng base, rng1, rng2;
    mco_rng_seed(&base, 42);

    rng1 = base;
    rng2 = base;
    mco_rng_jump(&rng2);

    /* Jumped RNG should produce different sequence */
    int same_count = 0;
    for (int i = 0; i < 100; i++) {
        if (mco_rng_next(&rng1) == mco_rng_next(&rng2)) {
            same_count++;
        }
    }
    TEST_ASSERT_EQUAL_INT(0, same_count);
}

static void test_rng_jump_reproducible(void)
{
    mco_rng base1, base2, rng1, rng2;

    /* Same seed + same jumps = same result */
    mco_rng_seed(&base1, 42);
    mco_rng_seed(&base2, 42);

    rng1 = base1;
    mco_rng_jump(&rng1);
    mco_rng_jump(&rng1);

    rng2 = base2;
    mco_rng_jump(&rng2);
    mco_rng_jump(&rng2);

    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_EQUAL_HEX64(mco_rng_next(&rng1), mco_rng_next(&rng2));
    }
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/
int main(void)
{
    UnityBegin("test_rng.c");

    RUN_TEST(test_rng_deterministic_seed);
    RUN_TEST(test_rng_different_seeds);
    RUN_TEST(test_rng_uniform_range);
    RUN_TEST(test_rng_uniform_mean);
    RUN_TEST(test_rng_normal_mean);
    RUN_TEST(test_rng_normal_variance);
    RUN_TEST(test_rng_jump_different_streams);
    RUN_TEST(test_rng_jump_reproducible);

    return UnityEnd();
}
