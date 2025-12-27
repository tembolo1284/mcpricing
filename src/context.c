/*
 * Context management
 *
 * The context holds all state for Monte Carlo simulation.
 * Each context is independent - thread-safe by isolation.
 */

#include "internal/context.h"
#include "internal/allocator.h"
#include "internal/rng.h"
#include <string.h>

/*============================================================================
 * Context Lifecycle
 *============================================================================*/

mco_ctx *mco_ctx_new(void)
{
    mco_ctx *ctx = (mco_ctx *)mco_calloc(1, sizeof(mco_ctx));
    if (!ctx) {
        return NULL;
    }

    /* Set defaults */
    ctx->num_simulations = MCO_DEFAULT_SIMULATIONS;
    ctx->num_steps       = MCO_DEFAULT_STEPS;
    ctx->seed            = MCO_DEFAULT_SEED;
    ctx->num_threads     = MCO_DEFAULT_THREADS;

    /* Variance reduction - all disabled by default */
    ctx->antithetic_enabled       = 0;
    ctx->control_variates_enabled = 0;
    ctx->stratified_enabled       = 0;

    /* Model - GBM by default */
    ctx->model = 0;

    /* SABR defaults (not used until model=SABR) */
    ctx->sabr_alpha = 0.0;
    ctx->sabr_beta  = 1.0;
    ctx->sabr_rho   = 0.0;
    ctx->sabr_nu    = 0.0;

    /* Initialize RNG with default seed */
    mco_rng_seed(&ctx->rng, ctx->seed);

    /* No errors yet */
    ctx->last_error = MCO_OK;

    return ctx;
}

void mco_ctx_free(mco_ctx *ctx)
{
    if (ctx) {
        mco_free(ctx);
    }
}

/*============================================================================
 * Simulation Parameters
 *============================================================================*/

void mco_set_simulations(mco_ctx *ctx, uint64_t n)
{
    if (ctx && n > 0) {
        ctx->num_simulations = n;
    }
}

uint64_t mco_get_simulations(const mco_ctx *ctx)
{
    return ctx ? ctx->num_simulations : 0;
}

void mco_set_steps(mco_ctx *ctx, uint64_t n)
{
    if (ctx && n > 0) {
        ctx->num_steps = n;
    }
}

uint64_t mco_get_steps(const mco_ctx *ctx)
{
    return ctx ? ctx->num_steps : 0;
}

void mco_set_seed(mco_ctx *ctx, uint64_t seed)
{
    if (ctx) {
        ctx->seed = seed;
        mco_rng_seed(&ctx->rng, seed);
    }
}

uint64_t mco_get_seed(const mco_ctx *ctx)
{
    return ctx ? ctx->seed : 0;
}

void mco_set_threads(mco_ctx *ctx, uint32_t n)
{
    if (ctx) {
        ctx->num_threads = (n > 0) ? n : 1;
    }
}

uint32_t mco_get_threads(const mco_ctx *ctx)
{
    return ctx ? ctx->num_threads : 0;
}

/*============================================================================
 * Variance Reduction
 *============================================================================*/

void mco_set_antithetic(mco_ctx *ctx, int enabled)
{
    if (ctx) {
        ctx->antithetic_enabled = enabled ? 1 : 0;
    }
}

int mco_get_antithetic(const mco_ctx *ctx)
{
    return ctx ? ctx->antithetic_enabled : 0;
}

/*============================================================================
 * Error Handling
 *============================================================================*/

mco_error mco_ctx_last_error(const mco_ctx *ctx)
{
    return ctx ? ctx->last_error : MCO_ERR_INVALID_ARG;
}

const char *mco_error_string(mco_error err)
{
    switch (err) {
        case MCO_OK:              return "Success";
        case MCO_ERR_NOMEM:       return "Out of memory";
        case MCO_ERR_INVALID_ARG: return "Invalid argument";
        case MCO_ERR_THREAD:      return "Threading error";
        default:                  return "Unknown error";
    }
}
