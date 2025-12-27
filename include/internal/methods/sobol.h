/*
 * Sobol Quasi-Random Sequences
 *
 * Sobol sequences are low-discrepancy sequences that fill space more
 * evenly than pseudo-random numbers. This gives faster convergence
 * for Monte Carlo integration (typically O(1/N) vs O(1/√N)).
 *
 * Properties:
 *   - Deterministic (not random)
 *   - Better space-filling than pseudo-random
 *   - Error ~ O(log(N)^d / N) vs O(1/√N) for pseudo-random
 *   - Works best for smooth integrands
 *
 * Implementation:
 *   Uses the gray code generation method for efficiency.
 *   Direction numbers from Joe & Kuo (2008) for high quality.
 *
 * Dimensions:
 *   Supports up to 21201 dimensions (standard tables).
 *   For options, typically need ~1-2 dimensions per time step.
 *
 * Usage:
 *   - Call mco_sobol_init() once
 *   - Call mco_sobol_next() to get next point in [0,1)^d
 *   - Apply inverse normal CDF to get quasi-random normals
 *
 * Reference:
 *   Sobol, I.M. (1967). Distribution of points in a cube
 *   Joe, S. & Kuo, F.Y. (2008). Constructing Sobol sequences
 */

#ifndef MCO_INTERNAL_METHODS_SOBOL_H
#define MCO_INTERNAL_METHODS_SOBOL_H

#include <stddef.h>
#include <stdint.h>

/* Maximum supported dimensions */
#define MCO_SOBOL_MAX_DIM 1024

/* Bits for precision (32-bit) */
#define MCO_SOBOL_BITS 32

/*
 * Sobol sequence state
 */
typedef struct {
    uint32_t dim;                       /* Number of dimensions */
    uint32_t count;                     /* Current index in sequence */
    uint32_t x[MCO_SOBOL_MAX_DIM];      /* Current point (as integers) */
    uint32_t v[MCO_SOBOL_MAX_DIM][MCO_SOBOL_BITS];  /* Direction numbers */
} mco_sobol;

/*
 * Initialize Sobol sequence generator
 *
 * Parameters:
 *   sobol - Sobol state to initialize
 *   dim   - Number of dimensions (1 to MCO_SOBOL_MAX_DIM)
 *
 * Returns:
 *   0 on success, -1 on error
 */
int mco_sobol_init(mco_sobol *sobol, uint32_t dim);

/*
 * Generate next point in sequence
 *
 * Parameters:
 *   sobol - Sobol state
 *   point - Output array of size dim (values in [0, 1))
 */
void mco_sobol_next(mco_sobol *sobol, double *point);

/*
 * Skip ahead in sequence (useful for parallel generation)
 *
 * Parameters:
 *   sobol - Sobol state
 *   n     - Number of points to skip
 */
void mco_sobol_skip(mco_sobol *sobol, uint64_t n);

/*
 * Reset sequence to beginning
 */
void mco_sobol_reset(mco_sobol *sobol);

/*
 * Convert uniform Sobol point to normal distribution
 * Uses inverse normal CDF (Moro's algorithm)
 */
double mco_sobol_inv_normal(double u);

/*
 * Generate next quasi-random normal vector
 *
 * Parameters:
 *   sobol  - Sobol state
 *   normal - Output array of standard normal values
 */
void mco_sobol_next_normal(mco_sobol *sobol, double *normal);

#endif /* MCO_INTERNAL_METHODS_SOBOL_H */
