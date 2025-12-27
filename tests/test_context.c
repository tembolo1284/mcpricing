/*
 * Context Tests
 *
 * Tests for context creation, configuration, and lifecycle.
 * Verifies:
 *   - Context allocation and free
 *   - Default values
 *   - Getters and setters
 *   - Error handling
 */

#include "unity/unity.h"
#include "mcoptions.h"

/*-------------------------------------------------------
 * Lifecycle Tests
 *-------------------------------------------------------*/

void test_context_new_not_null(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_NOT_NULL(ctx);
    mco_ctx_free(ctx);
}

void test_context_free_null_safe(void)
{
    /* Should not crash */
    mco_ctx_free(NULL);
    TEST_ASSERT_TRUE(1);
}

/*-------------------------------------------------------
 * Default Values
 *-------------------------------------------------------*/

void test_context_default_simulations(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_EQUAL_UINT64(100000, mco_get_simulations(ctx));
    mco_ctx_free(ctx);
}

void test_context_default_steps(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_EQUAL_UINT64(252, mco_get_steps(ctx));
    mco_ctx_free(ctx);
}

void test_context_default_threads(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_EQUAL_UINT(1, mco_get_threads(ctx));
    mco_ctx_free(ctx);
}

void test_context_default_antithetic(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_EQUAL_INT(0, mco_get_antithetic(ctx));
    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Setter/Getter Tests
 *-------------------------------------------------------*/

void test_context_set_simulations(void)
{
    mco_ctx *ctx = mco_ctx_new();

    mco_set_simulations(ctx, 500000);
    TEST_ASSERT_EQUAL_UINT64(500000, mco_get_simulations(ctx));

    mco_set_simulations(ctx, 1000);
    TEST_ASSERT_EQUAL_UINT64(1000, mco_get_simulations(ctx));

    mco_ctx_free(ctx);
}

void test_context_set_steps(void)
{
    mco_ctx *ctx = mco_ctx_new();

    mco_set_steps(ctx, 365);
    TEST_ASSERT_EQUAL_UINT64(365, mco_get_steps(ctx));

    mco_set_steps(ctx, 52);
    TEST_ASSERT_EQUAL_UINT64(52, mco_get_steps(ctx));

    mco_ctx_free(ctx);
}

void test_context_set_threads(void)
{
    mco_ctx *ctx = mco_ctx_new();

    mco_set_threads(ctx, 8);
    TEST_ASSERT_EQUAL_UINT(8, mco_get_threads(ctx));

    mco_set_threads(ctx, 1);
    TEST_ASSERT_EQUAL_UINT(1, mco_get_threads(ctx));

    mco_ctx_free(ctx);
}

void test_context_set_threads_zero_becomes_one(void)
{
    mco_ctx *ctx = mco_ctx_new();

    mco_set_threads(ctx, 0);
    TEST_ASSERT_EQUAL_UINT(1, mco_get_threads(ctx));

    mco_ctx_free(ctx);
}

void test_context_set_seed(void)
{
    mco_ctx *ctx = mco_ctx_new();

    mco_set_seed(ctx, 12345);
    TEST_ASSERT_EQUAL_UINT64(12345, mco_get_seed(ctx));

    mco_set_seed(ctx, 0);
    TEST_ASSERT_EQUAL_UINT64(0, mco_get_seed(ctx));

    mco_ctx_free(ctx);
}

void test_context_set_antithetic(void)
{
    mco_ctx *ctx = mco_ctx_new();

    mco_set_antithetic(ctx, 1);
    TEST_ASSERT_EQUAL_INT(1, mco_get_antithetic(ctx));

    mco_set_antithetic(ctx, 0);
    TEST_ASSERT_EQUAL_INT(0, mco_get_antithetic(ctx));

    /* Non-zero becomes 1 */
    mco_set_antithetic(ctx, 42);
    TEST_ASSERT_EQUAL_INT(1, mco_get_antithetic(ctx));

    mco_ctx_free(ctx);
}

/*-------------------------------------------------------
 * Null Safety Tests
 *-------------------------------------------------------*/

void test_context_getters_null_safe(void)
{
    TEST_ASSERT_EQUAL_UINT64(0, mco_get_simulations(NULL));
    TEST_ASSERT_EQUAL_UINT64(0, mco_get_steps(NULL));
    TEST_ASSERT_EQUAL_UINT64(0, mco_get_seed(NULL));
    TEST_ASSERT_EQUAL_UINT(0, mco_get_threads(NULL));
    TEST_ASSERT_EQUAL_INT(0, mco_get_antithetic(NULL));
}

void test_context_setters_null_safe(void)
{
    /* Should not crash */
    mco_set_simulations(NULL, 100);
    mco_set_steps(NULL, 100);
    mco_set_seed(NULL, 100);
    mco_set_threads(NULL, 4);
    mco_set_antithetic(NULL, 1);
    TEST_ASSERT_TRUE(1);
}

/*-------------------------------------------------------
 * Error Handling Tests
 *-------------------------------------------------------*/

void test_context_error_default(void)
{
    mco_ctx *ctx = mco_ctx_new();
    TEST_ASSERT_EQUAL_INT(MCO_OK, mco_ctx_last_error(ctx));
    mco_ctx_free(ctx);
}

void test_error_string(void)
{
    TEST_ASSERT_EQUAL_STRING("Success", mco_error_string(MCO_OK));
    TEST_ASSERT_EQUAL_STRING("Out of memory", mco_error_string(MCO_ERR_NOMEM));
    TEST_ASSERT_EQUAL_STRING("Invalid argument", mco_error_string(MCO_ERR_INVALID_ARG));
    TEST_ASSERT_EQUAL_STRING("Threading error", mco_error_string(MCO_ERR_THREAD));
}

/*-------------------------------------------------------
 * Version Tests
 *-------------------------------------------------------*/

void test_version_number(void)
{
    uint32_t version = mco_version();

    /* Version 2.0.0 = (2 << 16) | (0 << 8) | 0 = 0x020000 */
    TEST_ASSERT_EQUAL_HEX(0x020000, version);
}

void test_version_string(void)
{
    const char *version = mco_version_string();
    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_EQUAL_STRING("mcoptions 2.0.0", version);
}

void test_version_compatible(void)
{
    TEST_ASSERT_EQUAL_INT(1, mco_is_compatible());
}

/*-------------------------------------------------------
 * Test Runner
 *-------------------------------------------------------*/

int main(void)
{
    UnityBegin("test_context.c");

    /* Lifecycle */
    RUN_TEST(test_context_new_not_null);
    RUN_TEST(test_context_free_null_safe);

    /* Defaults */
    RUN_TEST(test_context_default_simulations);
    RUN_TEST(test_context_default_steps);
    RUN_TEST(test_context_default_threads);
    RUN_TEST(test_context_default_antithetic);

    /* Setters/Getters */
    RUN_TEST(test_context_set_simulations);
    RUN_TEST(test_context_set_steps);
    RUN_TEST(test_context_set_threads);
    RUN_TEST(test_context_set_threads_zero_becomes_one);
    RUN_TEST(test_context_set_seed);
    RUN_TEST(test_context_set_antithetic);

    /* Null safety */
    RUN_TEST(test_context_getters_null_safe);
    RUN_TEST(test_context_setters_null_safe);

    /* Error handling */
    RUN_TEST(test_context_error_default);
    RUN_TEST(test_error_string);

    /* Version */
    RUN_TEST(test_version_number);
    RUN_TEST(test_version_string);
    RUN_TEST(test_version_compatible);

    return UnityEnd();
}
