/*
 * Monte Carlo Options Pricing Library
 * 
 * A high-performance options pricing library using Monte Carlo simulation
 * with multi-threaded execution and variance reduction techniques.
 *
 * Design principles:
 *   - Single public header, opaque types, no global state
 *   - All state lives in context objects
 *   - Thread-safe: each context is independent
 *   - Reproducible: same seed + thread count = same results
 */

#ifndef MCOPTIONS_H
#define MCOPTIONS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * API Export Macros
 *============================================================================*/

#ifndef MCO_API
#  ifdef _WIN32
#    if defined(MCO_BUILD_SHARED)
#      define MCO_API __declspec(dllexport)
#    elif !defined(MCO_BUILD_STATIC)
#      define MCO_API __declspec(dllimport)
#    else
#      define MCO_API
#    endif
#  else
#    if __GNUC__ >= 4
#      define MCO_API __attribute__((visibility("default")))
#    else
#      define MCO_API
#    endif
#  endif
#endif

/*============================================================================
 * Version
 *============================================================================*/

#define MCO_VERSION_MAJOR 2
#define MCO_VERSION_MINOR 0
#define MCO_VERSION_PATCH 0
#define MCO_VERSION ((MCO_VERSION_MAJOR << 16) | (MCO_VERSION_MINOR << 8) | MCO_VERSION_PATCH)

MCO_API uint32_t mco_version(void);
MCO_API const char *mco_version_string(void);
MCO_API int mco_is_compatible(void);

/*============================================================================
 * Memory Allocation
 *============================================================================*/

typedef void *(*mco_malloc_fn)(size_t size);
typedef void *(*mco_realloc_fn)(void *ptr, size_t size);
typedef void  (*mco_free_fn)(void *ptr);

MCO_API void mco_set_allocators(mco_malloc_fn f_malloc,
                                 mco_realloc_fn f_realloc,
                                 mco_free_fn f_free);

/*============================================================================
 * Context
 *============================================================================*/

typedef struct mco_ctx mco_ctx;

MCO_API mco_ctx *mco_ctx_new(void);
MCO_API void     mco_ctx_free(mco_ctx *ctx);

/* Simulation parameters */
MCO_API void   mco_set_simulations(mco_ctx *ctx, uint64_t n);
MCO_API void   mco_set_steps(mco_ctx *ctx, uint64_t n);
MCO_API void   mco_set_seed(mco_ctx *ctx, uint64_t seed);
MCO_API void   mco_set_threads(mco_ctx *ctx, uint32_t n);

MCO_API uint64_t mco_get_simulations(const mco_ctx *ctx);
MCO_API uint64_t mco_get_steps(const mco_ctx *ctx);
MCO_API uint64_t mco_get_seed(const mco_ctx *ctx);
MCO_API uint32_t mco_get_threads(const mco_ctx *ctx);

/* Variance reduction (to be added incrementally) */
MCO_API void mco_set_antithetic(mco_ctx *ctx, int enabled);
MCO_API int  mco_get_antithetic(const mco_ctx *ctx);

/*============================================================================
 * Types
 *============================================================================*/

#ifndef MCO_OPTION_TYPE_DEFINED
#define MCO_OPTION_TYPE_DEFINED
typedef enum {
    MCO_CALL = 0,
    MCO_PUT  = 1
} mco_option_type;
#endif

/*============================================================================
 * European Options
 *============================================================================*/

/* Black-Scholes analytical formulas */
MCO_API double mco_black_scholes_call(double spot, double strike, double rate,
                                       double volatility, double time);

MCO_API double mco_black_scholes_put(double spot, double strike, double rate,
                                      double volatility, double time);

/* Monte Carlo pricing */
MCO_API double mco_european_call(mco_ctx *ctx,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity);

MCO_API double mco_european_put(mco_ctx *ctx,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double volatility,
                                 double time_to_maturity);

/*============================================================================
 * American Options (Least Squares Monte Carlo)
 *============================================================================*/

MCO_API double mco_american_call(mco_ctx *ctx,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity,
                                  size_t num_steps);

MCO_API double mco_american_put(mco_ctx *ctx,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double volatility,
                                 double time_to_maturity,
                                 size_t num_steps);

/*============================================================================
 * Asian Options (Arithmetic Average)
 *============================================================================*/

MCO_API double mco_asian_call(mco_ctx *ctx,
                               double spot,
                               double strike,
                               double rate,
                               double volatility,
                               double time_to_maturity,
                               size_t num_obs);

MCO_API double mco_asian_put(mco_ctx *ctx,
                              double spot,
                              double strike,
                              double rate,
                              double volatility,
                              double time_to_maturity,
                              size_t num_obs);

MCO_API double mco_asian_geometric_call(mco_ctx *ctx,
                                         double spot,
                                         double strike,
                                         double rate,
                                         double volatility,
                                         double time_to_maturity,
                                         size_t num_obs);

MCO_API double mco_asian_geometric_put(mco_ctx *ctx,
                                        double spot,
                                        double strike,
                                        double rate,
                                        double volatility,
                                        double time_to_maturity,
                                        size_t num_obs);

/* Closed-form geometric Asian (for validation) */
MCO_API double mco_asian_geometric_closed(double spot,
                                           double strike,
                                           double rate,
                                           double volatility,
                                           double time_to_maturity,
                                           size_t num_obs,
                                           mco_option_type type);

/*============================================================================
 * Bermudan Options (Discrete Early Exercise)
 *============================================================================*/

MCO_API double mco_bermudan_call(mco_ctx *ctx,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity,
                                  size_t num_exercise);

MCO_API double mco_bermudan_put(mco_ctx *ctx,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double volatility,
                                 double time_to_maturity,
                                 size_t num_exercise);

/*============================================================================
 * SABR Model Functions
 *============================================================================*/

MCO_API void mco_set_sabr(mco_ctx *ctx,
                           double alpha,
                           double beta,
                           double rho,
                           double nu);

MCO_API double mco_sabr_implied_vol(double forward,
                                     double strike,
                                     double time,
                                     double alpha,
                                     double beta,
                                     double rho,
                                     double nu);

MCO_API double mco_sabr_european_call(mco_ctx *ctx,
                                       double forward,
                                       double strike,
                                       double rate,
                                       double time_to_maturity,
                                       double alpha,
                                       double beta,
                                       double rho,
                                       double nu);

MCO_API double mco_sabr_european_put(mco_ctx *ctx,
                                      double forward,
                                      double strike,
                                      double rate,
                                      double time_to_maturity,
                                      double alpha,
                                      double beta,
                                      double rho,
                                      double nu);

/*============================================================================
 * Black-76 Model (Futures/Forward Options)
 *============================================================================*/

MCO_API double mco_black76_call(double forward,
                                 double strike,
                                 double rate,
                                 double volatility,
                                 double time);

MCO_API double mco_black76_put(double forward,
                                double strike,
                                double rate,
                                double volatility,
                                double time);

MCO_API double mco_black76_implied_vol(double forward,
                                        double strike,
                                        double rate,
                                        double time,
                                        double price,
                                        int is_call);

/* Black-76 Greeks */
MCO_API double mco_black76_delta(double forward, double strike, double rate,
                                  double volatility, double time, int is_call);

MCO_API double mco_black76_gamma(double forward, double strike, double rate,
                                  double volatility, double time);

MCO_API double mco_black76_vega(double forward, double strike, double rate,
                                 double volatility, double time);

MCO_API double mco_black76_theta(double forward, double strike, double rate,
                                  double volatility, double time, int is_call);

/*============================================================================
 * Heston Stochastic Volatility Model
 *============================================================================*/

MCO_API double mco_heston_european_call(mco_ctx *ctx,
                                         double spot,
                                         double strike,
                                         double rate,
                                         double time,
                                         double v0,
                                         double kappa,
                                         double theta,
                                         double sigma,
                                         double rho);

MCO_API double mco_heston_european_put(mco_ctx *ctx,
                                        double spot,
                                        double strike,
                                        double rate,
                                        double time,
                                        double v0,
                                        double kappa,
                                        double theta,
                                        double sigma,
                                        double rho);

MCO_API int mco_heston_check_feller(double kappa, double theta, double sigma);

/*============================================================================
 * Merton Jump-Diffusion Model
 *============================================================================*/

/* Analytical (series expansion) */
MCO_API double mco_merton_call(double spot, double strike, double rate, double time,
                                double sigma, double lambda, double mu_j, double sigma_j);

MCO_API double mco_merton_put(double spot, double strike, double rate, double time,
                               double sigma, double lambda, double mu_j, double sigma_j);

/* Monte Carlo */
MCO_API double mco_merton_european_call(mco_ctx *ctx,
                                         double spot,
                                         double strike,
                                         double rate,
                                         double time,
                                         double sigma,
                                         double lambda,
                                         double mu_j,
                                         double sigma_j);

MCO_API double mco_merton_european_put(mco_ctx *ctx,
                                        double spot,
                                        double strike,
                                        double rate,
                                        double time,
                                        double sigma,
                                        double lambda,
                                        double mu_j,
                                        double sigma_j);

/*============================================================================
 * Barrier Options
 *============================================================================*/

typedef enum {
    MCO_BARRIER_DOWN_IN  = 0,
    MCO_BARRIER_DOWN_OUT = 1,
    MCO_BARRIER_UP_IN    = 2,
    MCO_BARRIER_UP_OUT   = 3
} mco_barrier_style;

MCO_API double mco_barrier_call(mco_ctx *ctx, double spot, double strike,
                                 double barrier, double rebate, double rate,
                                 double vol, double time, size_t steps,
                                 mco_barrier_style type);

MCO_API double mco_barrier_put(mco_ctx *ctx, double spot, double strike,
                                double barrier, double rebate, double rate,
                                double vol, double time, size_t steps,
                                mco_barrier_style type);

/* Analytical barrier formulas (continuous monitoring) */
MCO_API double mco_barrier_down_out_call(double spot, double strike, double barrier,
                                          double rebate, double rate, double vol, double time);
MCO_API double mco_barrier_down_in_call(double spot, double strike, double barrier,
                                         double rebate, double rate, double vol, double time);
MCO_API double mco_barrier_up_out_call(double spot, double strike, double barrier,
                                        double rebate, double rate, double vol, double time);
MCO_API double mco_barrier_up_in_call(double spot, double strike, double barrier,
                                       double rebate, double rate, double vol, double time);

/*============================================================================
 * Lookback Options
 *============================================================================*/

MCO_API double mco_lookback_call(mco_ctx *ctx, double spot, double strike,
                                  double rate, double vol, double time,
                                  size_t steps, int floating_strike);

MCO_API double mco_lookback_put(mco_ctx *ctx, double spot, double strike,
                                 double rate, double vol, double time,
                                 size_t steps, int floating_strike);

/* Analytical lookback formulas */
MCO_API double mco_lookback_floating_call(double spot, double rate, double vol, double time);
MCO_API double mco_lookback_floating_put(double spot, double rate, double vol, double time);

/*============================================================================
 * Digital (Binary) Options
 *============================================================================*/

MCO_API double mco_digital_call(mco_ctx *ctx, double spot, double strike,
                                 double payout, double rate, double vol,
                                 double time, int cash_or_nothing);

MCO_API double mco_digital_put(mco_ctx *ctx, double spot, double strike,
                                double payout, double rate, double vol,
                                double time, int cash_or_nothing);

/* Analytical digital formulas */
MCO_API double mco_digital_cash_call(double spot, double strike, double payout,
                                      double rate, double vol, double time);
MCO_API double mco_digital_cash_put(double spot, double strike, double payout,
                                     double rate, double vol, double time);
MCO_API double mco_digital_asset_call(double spot, double strike,
                                       double rate, double vol, double time);
MCO_API double mco_digital_asset_put(double spot, double strike,
                                      double rate, double vol, double time);

/*============================================================================
 * Control Variates (Variance Reduction)
 *============================================================================*/

/* European with spot control variate */
MCO_API double mco_european_call_cv(mco_ctx *ctx,
                                     double spot,
                                     double strike,
                                     double rate,
                                     double volatility,
                                     double time_to_maturity);

MCO_API double mco_european_put_cv(mco_ctx *ctx,
                                    double spot,
                                    double strike,
                                    double rate,
                                    double volatility,
                                    double time_to_maturity);

/* Asian with geometric control variate */
MCO_API double mco_asian_call_cv(mco_ctx *ctx,
                                  double spot,
                                  double strike,
                                  double rate,
                                  double volatility,
                                  double time_to_maturity,
                                  size_t num_obs);

MCO_API double mco_asian_put_cv(mco_ctx *ctx,
                                 double spot,
                                 double strike,
                                 double rate,
                                 double volatility,
                                 double time_to_maturity,
                                 size_t num_obs);

/*============================================================================
 * Error Handling
 *============================================================================*/

typedef enum {
    MCO_OK = 0,
    MCO_ERR_NOMEM,
    MCO_ERR_INVALID_ARG,
    MCO_ERR_THREAD
} mco_error;

MCO_API mco_error   mco_ctx_last_error(const mco_ctx *ctx);
MCO_API const char *mco_error_string(mco_error err);

#ifdef __cplusplus
}
#endif

#endif /* MCOPTIONS_H */
