/*
 * Thread Pool for Parallel Monte Carlo Simulation
 *
 * Design:
 *   - Spawn N threads, each processes 1/N of simulations
 *   - Each thread has its own RNG (jumped from base for reproducibility)
 *   - No locks during simulation - each thread writes to its own slot
 *   - Single reduction at the end to sum partial results
 *
 * Reproducibility:
 *   - Same seed + same thread count = same results
 *   - Thread i uses RNG state = jump(base_rng, i times)
 */

#include "internal/methods/thread_pool.h"
#include "internal/models/gbm.h"
#include "internal/instruments/payoff.h"
#include "internal/variance_reduction/antithetic.h"
#include "internal/allocator.h"

#include <pthread.h>
#include <string.h>

/*============================================================================
 * Thread Work Initialization
 *============================================================================*/

void mco_thread_work_init(mco_thread_work *work,
                          uint32_t num_threads,
                          const mco_rng *base_rng,
                          uint64_t total_sims)
{
    uint64_t sims_per_thread = total_sims / num_threads;
    uint64_t remainder = total_sims % num_threads;

    /* Copy base RNG state */
    mco_rng rng = *base_rng;

    uint64_t start = 0;
    for (uint32_t i = 0; i < num_threads; ++i) {
        /* Distribute remainder among first threads */
        uint64_t count = sims_per_thread + (i < remainder ? 1 : 0);

        work[i].rng = rng;
        work[i].start_sim = start;
        work[i].end_sim = start + count;
        work[i].partial_sum = 0.0;
        work[i].partial_sum_sq = 0.0;

        start += count;

        /* Jump RNG for next thread */
        mco_rng_jump(&rng);
    }
}

/*============================================================================
 * Worker Thread Functions
 *============================================================================*/

/*
 * Worker function for basic European pricing (no variance reduction).
 */
static void *worker_european_basic(void *arg)
{
    mco_thread_work *work = (mco_thread_work *)arg;

    mco_gbm model;
    mco_gbm_init(&model, work->spot, work->rate, work->volatility, 
                 work->time_to_maturity);

    double sum = 0.0;
    mco_option_type type = (mco_option_type)work->option_type;

    for (uint64_t i = work->start_sim; i < work->end_sim; ++i) {
        double s_t = mco_gbm_simulate(&model, &work->rng);
        sum += mco_payoff(s_t, work->strike, type);
    }

    work->partial_sum = sum;
    return NULL;
}

/*
 * Worker function for European pricing with antithetic variates.
 */
static void *worker_european_antithetic(void *arg)
{
    mco_thread_work *work = (mco_thread_work *)arg;

    mco_gbm model;
    mco_gbm_init(&model, work->spot, work->rate, work->volatility, 
                 work->time_to_maturity);

    mco_option_type type = (mco_option_type)work->option_type;

    /* Number of simulations for this thread */
    uint64_t n = work->end_sim - work->start_sim;
    uint64_t num_pairs = n / 2;
    if (num_pairs == 0) num_pairs = 1;

    double sum = mco_antithetic_european_sum(&model, &work->rng, work->strike,
                                              type, num_pairs);

    work->partial_sum = sum;
    /* Store actual number of paths simulated (2 per pair) */
    work->end_sim = work->start_sim + (2 * num_pairs);
    return NULL;
}

/*============================================================================
 * Parallel Execution
 *============================================================================*/

double mco_parallel_european(mco_ctx *ctx,
                             double spot,
                             double strike,
                             double rate,
                             double volatility,
                             double time_to_maturity,
                             int type)
{
    uint32_t num_threads = ctx->num_threads;
    uint64_t total_sims = ctx->num_simulations;

    /* Allocate work items */
    mco_thread_work *work = (mco_thread_work *)mco_calloc(num_threads, 
                                                          sizeof(mco_thread_work));
    if (!work) {
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Allocate thread handles */
    pthread_t *threads = (pthread_t *)mco_malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        mco_free(work);
        ctx->last_error = MCO_ERR_NOMEM;
        return 0.0;
    }

    /* Initialize work items */
    mco_thread_work_init(work, num_threads, &ctx->rng, total_sims);

    /* Set option parameters for all workers */
    for (uint32_t i = 0; i < num_threads; ++i) {
        work[i].spot = spot;
        work[i].strike = strike;
        work[i].rate = rate;
        work[i].volatility = volatility;
        work[i].time_to_maturity = time_to_maturity;
        work[i].option_type = type;
        work[i].antithetic = ctx->antithetic_enabled;
    }

    /* Select worker function */
    void *(*worker_fn)(void *) = ctx->antithetic_enabled 
                                  ? worker_european_antithetic 
                                  : worker_european_basic;

    /* Spawn threads */
    for (uint32_t i = 0; i < num_threads; ++i) {
        int rc = pthread_create(&threads[i], NULL, worker_fn, &work[i]);
        if (rc != 0) {
            /* Thread creation failed - wait for already-started threads */
            for (uint32_t j = 0; j < i; ++j) {
                pthread_join(threads[j], NULL);
            }
            mco_free(threads);
            mco_free(work);
            ctx->last_error = MCO_ERR_THREAD;
            return 0.0;
        }
    }

    /* Wait for all threads */
    for (uint32_t i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    /* Reduce results */
    double total_sum = 0.0;
    uint64_t total_paths = 0;

    for (uint32_t i = 0; i < num_threads; ++i) {
        total_sum += work[i].partial_sum;
        total_paths += (work[i].end_sim - work[i].start_sim);
    }

    /* Calculate discount factor and final price */
    double discount = exp(-rate * time_to_maturity);
    double price = discount * (total_sum / (double)total_paths);

    /* Cleanup */
    mco_free(threads);
    mco_free(work);

    return price;
}
