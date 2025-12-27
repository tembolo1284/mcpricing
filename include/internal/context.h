/*
 * Internal context structure
 *
 * This is the private definition of mco_ctx. Users only see
 * an opaque pointer; this header is for internal use only.
 */

#ifndef MCO_INTERNAL_CONTEXT_H
#define MCO_INTERNAL_CONTEXT_H

#include "mcoptions.h"
#include "internal/rng.h"
#include <stdint.h>

/*
 * Context holds all state for Monte Carlo simulation.
 * Each context is independent - no shared state between contexts.
 */
struct mco_ctx {
    /* Simulation parameters */
    uint64_t num_simulations;       /* Number of MC paths (default: 100000) */
    uint64_t num_steps;             /* Time steps per path (default: 252) */
    uint64_t seed;                  /* Master RNG seed */
    uint32_t num_threads;           /* Thread count (default: 1) */

    /* Variance reduction flags */
    int antithetic_enabled;         /* Antithetic variates */
    int control_variates_enabled;   /* Control variates (future) */
    int stratified_enabled;         /* Stratified sampling (future) */

    /* Model selection (future) */
    int model;                      /* 0=GBM, 1=Heston, 2=SABR */

    /* SABR parameters (future) */
    double sabr_alpha;
    double sabr_beta;
    double sabr_rho;
    double sabr_nu;

    /* Master RNG - thread RNGs are derived from this via jump() */
    mco_rng rng;

    /* Error state */
    mco_error last_error;
};

/*
 * Default values for new contexts
 */
#define MCO_DEFAULT_SIMULATIONS  100000
#define MCO_DEFAULT_STEPS        252
#define MCO_DEFAULT_SEED         0xDEADBEEF
#define MCO_DEFAULT_THREADS      1

#endif /* MCO_INTERNAL_CONTEXT_H */
