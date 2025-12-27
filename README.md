# mcoptions

**A production-grade Monte Carlo options pricing library in pure C.**

---

**Version 2.5.0** | **140 tests** | **5 models** | **10 instruments** | **C11 + pthreads**

## Features

### Stochastic Models
- **GBM** - Geometric Brownian Motion with Black-Scholes analytical
- **SABR** - Stochastic Alpha Beta Rho (Hagan formula + MC)
- **Heston** - Stochastic variance with mean reversion
- **Merton** - Jump-diffusion with Poisson jumps
- **Black-76** - Futures/forwards with Greeks

### Option Instruments
- **European** - Vanilla calls/puts
- **American** - Early exercise via Least Squares Monte Carlo
- **Asian** - Arithmetic and geometric averaging
- **Bermudan** - Discrete exercise dates
- **Barrier** - Knock-in/knock-out with Brownian bridge
- **Lookback** - Floating and fixed strike
- **Digital** - Cash-or-nothing, asset-or-nothing

### Monte Carlo Methods
- **Pseudo-random** - Xoshiro256** (period 2²⁵⁶)
- **Quasi-random** - Sobol low-discrepancy sequences
- **Variance reduction** - Antithetic variates, control variates
- **Parallelization** - Thread pool with independent RNG streams
- **LSM** - Longstaff-Schwartz regression for early exercise

---

## Quick Start

```bash
# Build
make

# Test (140 tests)
make run-tests

# Install
sudo make install
```

### Example

```c
#include <mcoptions.h>
#include <stdio.h>

int main(void) {
    mco_ctx *ctx = mco_ctx_new();
    mco_set_simulations(ctx, 100000);
    mco_set_threads(ctx, 4);
    mco_set_antithetic(ctx, 1);

    // European call (Black-Scholes comparison)
    double bs = mco_black_scholes_call(100, 100, 0.05, 0.20, 1.0);
    double mc = mco_european_call(ctx, 100, 100, 0.05, 0.20, 1.0);
    printf("BS: $%.2f  MC: $%.2f\n", bs, mc);

    // American put with early exercise
    double american = mco_american_put(ctx, 100, 100, 0.05, 0.20, 1.0, 52);
    printf("American Put: $%.2f\n", american);

    // Down-and-out barrier
    double barrier = mco_barrier_call(ctx, 100, 100, 80, 0, 0.05, 0.20, 1.0, 
                                       252, MCO_BARRIER_DOWN_OUT);
    printf("Barrier Call: $%.2f\n", barrier);

    // Heston stochastic volatility
    double heston = mco_heston_european_call(ctx, 100, 100, 0.05, 1.0,
                                              0.04, 2.0, 0.04, 0.3, -0.7);
    printf("Heston Call: $%.2f\n", heston);

    mco_ctx_free(ctx);
    return 0;
}
```

Compile: `gcc example.c -lmcoptions -lm -lpthread -o example`

---

## Project Structure

```
mcoptions/
├── include/
│   ├── mcoptions.h                      # Public API (single header)
│   └── internal/
│       ├── allocator.h                  # Custom memory allocation
│       ├── context.h                    # Simulation context
│       ├── rng.h                        # Xoshiro256** RNG
│       ├── models/
│       │   ├── gbm.h                    # Geometric Brownian Motion
│       │   ├── sabr.h                   # SABR stochastic vol
│       │   ├── heston.h                 # Heston stochastic vol
│       │   ├── merton_jump.h            # Merton jump-diffusion
│       │   └── black76.h                # Black-76 futures
│       ├── instruments/
│       │   ├── payoff.h                 # Payoff functions
│       │   ├── european.h               # European options
│       │   ├── american.h               # American options (LSM)
│       │   ├── asian.h                  # Asian options
│       │   ├── bermudan.h               # Bermudan options
│       │   ├── barrier.h                # Barrier options
│       │   ├── lookback.h               # Lookback options
│       │   └── digital.h                # Digital/binary options
│       ├── methods/
│       │   ├── monte_carlo.h            # MC framework
│       │   ├── thread_pool.h            # Parallel execution
│       │   ├── lsm.h                    # Least Squares MC
│       │   └── sobol.h                  # Quasi-random sequences
│       └── variance_reduction/
│           ├── antithetic.h             # Antithetic variates
│           └── control_variates.h       # Control variates
├── src/
│   ├── allocator.c
│   ├── context.c
│   ├── rng.c
│   ├── version.c
│   ├── models/
│   │   ├── gbm.c
│   │   ├── sabr.c
│   │   ├── sabr_pricing.c
│   │   ├── heston.c
│   │   ├── merton_jump.c
│   │   └── black76.c
│   ├── instruments/
│   │   ├── european.c
│   │   ├── american.c
│   │   ├── asian.c
│   │   ├── bermudan.c
│   │   ├── barrier.c
│   │   ├── lookback.c
│   │   └── digital.c
│   ├── methods/
│   │   ├── thread_pool.c
│   │   ├── lsm.c
│   │   └── sobol.c
│   └── variance_reduction/
│       └── control_variates.c
├── tests/
│   ├── unity/                           # Unity test framework
│   │   ├── unity.h
│   │   └── unity.c
│   ├── test_rng.c                       # 8 tests
│   ├── test_context.c                   # 19 tests
│   ├── test_european.c                  # 15 tests
│   ├── test_american.c                  # 12 tests
│   ├── test_asian.c                     # 7 tests
│   ├── test_bermudan.c                  # 7 tests
│   ├── test_sabr.c                      # 9 tests
│   ├── test_black76.c                   # 16 tests
│   ├── test_control_variates.c          # 8 tests
│   ├── test_heston.c                    # 8 tests
│   ├── test_merton.c                    # 10 tests
│   ├── test_barrier.c                   # 6 tests
│   ├── test_lookback.c                  # 6 tests
│   └── test_digital.c                   # 9 tests
├── build/
│   ├── libmcoptions.so                  # Shared library
│   └── libmcoptions.a                   # Static library
├── Makefile
└── README.md
```

---

## API Reference

### Context Management
```c
mco_ctx *mco_ctx_new(void);
void mco_ctx_free(mco_ctx *ctx);
void mco_set_simulations(mco_ctx *ctx, uint64_t n);
void mco_set_steps(mco_ctx *ctx, uint64_t n);
void mco_set_seed(mco_ctx *ctx, uint64_t seed);
void mco_set_threads(mco_ctx *ctx, uint32_t n);
void mco_set_antithetic(mco_ctx *ctx, int enable);
```

### European Options
```c
double mco_european_call(ctx, spot, strike, rate, vol, time);
double mco_european_put(ctx, spot, strike, rate, vol, time);
double mco_black_scholes_call(spot, strike, rate, vol, time);
double mco_black_scholes_put(spot, strike, rate, vol, time);
```

### American Options
```c
double mco_american_call(ctx, spot, strike, rate, vol, time, steps);
double mco_american_put(ctx, spot, strike, rate, vol, time, steps);
```

### Asian Options
```c
double mco_asian_call(ctx, spot, strike, rate, vol, time, observations);
double mco_asian_put(ctx, spot, strike, rate, vol, time, observations);
double mco_asian_geometric_call(ctx, spot, strike, rate, vol, time, observations);
double mco_asian_call_cv(ctx, ...);  // With control variate
```

### Bermudan Options
```c
double mco_bermudan_call(ctx, spot, strike, rate, vol, time, exercise_dates);
double mco_bermudan_put(ctx, spot, strike, rate, vol, time, exercise_dates);
```

### Barrier Options
```c
double mco_barrier_call(ctx, spot, strike, barrier, rebate, rate, vol, time, 
                        steps, MCO_BARRIER_DOWN_OUT);
double mco_barrier_put(ctx, ...);
// Analytical: mco_barrier_down_out_call(), mco_barrier_up_in_call(), etc.
```

### Lookback Options
```c
double mco_lookback_call(ctx, spot, strike, rate, vol, time, steps, floating);
double mco_lookback_put(ctx, spot, strike, rate, vol, time, steps, floating);
// Analytical: mco_lookback_floating_call(), mco_lookback_fixed_call()
```

### Digital Options
```c
double mco_digital_call(ctx, spot, strike, payout, rate, vol, time, cash_or_nothing);
double mco_digital_put(ctx, ...);
// Analytical: mco_digital_cash_call(), mco_digital_asset_call()
```

### Heston Model
```c
double mco_heston_european_call(ctx, spot, strike, rate, time,
                                 v0, kappa, theta, sigma, rho);
double mco_heston_european_put(ctx, ...);
int mco_heston_check_feller(kappa, theta, sigma);
```

### Merton Jump-Diffusion
```c
double mco_merton_call(spot, strike, rate, time, sigma, lambda, mu_j, sigma_j);
double mco_merton_put(spot, strike, rate, time, sigma, lambda, mu_j, sigma_j);
double mco_merton_european_call(ctx, ...);  // MC version
```

### SABR Model
```c
double mco_sabr_implied_vol(forward, strike, time, alpha, beta, rho, nu);
double mco_sabr_european_call(ctx, forward, strike, rate, time, alpha, beta, rho, nu);
```

### Black-76
```c
double mco_black76_call(forward, strike, rate, vol, time);
double mco_black76_put(forward, strike, rate, vol, time);
double mco_black76_delta(forward, strike, rate, vol, time, is_call);
double mco_black76_gamma(forward, strike, rate, vol, time);
double mco_black76_vega(forward, strike, rate, vol, time);
double mco_black76_implied_vol(forward, strike, rate, time, price, is_call);
```

---

## Build Options

```bash
make                      # Release build (optimized)
make BUILD=debug          # Debug build with sanitizers
make run-tests            # Build and run 140 tests
make install              # Install to /usr/local
make PREFIX=/opt install  # Install to custom prefix
make clean                # Remove build artifacts
make help                 # Show all options
```

---

## Design Principles

1. **Single public header** - Just `#include <mcoptions.h>`
2. **Opaque types** - Implementation hidden, ABI stable
3. **No global state** - All state in context objects
4. **Thread-safe** - Contexts are independent
5. **Reproducible** - Same seed = same results
6. **Custom allocators** - Plug in your own malloc/free

---

## Performance Tips

| Technique | Variance Reduction | Cost |
|-----------|-------------------|------|
| Antithetic | ~50% | Free |
| Control variate (spot) | ~30-50% | Low |
| Control variate (geometric Asian) | ~80-95% | Low |
| More simulations | √N | Linear |
| Multithreading | - | Sublinear |
| Quasi-Monte Carlo | Better convergence | Low |

```c
// Optimal configuration for production
mco_set_simulations(ctx, 1000000);
mco_set_threads(ctx, 8);
mco_set_antithetic(ctx, 1);
mco_asian_call_cv(ctx, ...);  // Use control variate
```

---

## References

- Black, F. & Scholes, M. (1973). "The Pricing of Options and Corporate Liabilities"
- Black, F. (1976). "The Pricing of Commodity Contracts"
- Heston, S. (1993). "A Closed-Form Solution for Options with Stochastic Volatility"
- Merton, R.C. (1976). "Option Pricing when Underlying Stock Returns are Discontinuous"
- Longstaff, F.A. & Schwartz, E.S. (2001). "Valuing American Options by Simulation"
- Hagan, P.S. et al. (2002). "Managing Smile Risk"
- Blackman, D. & Vigna, S. (2018). "Scrambled Linear Pseudorandom Number Generators"
- Glasserman, P. (2003). "Monte Carlo Methods in Financial Engineering"

---

