/*
 * Sobol Quasi-Random Sequence Implementation
 *
 * Uses gray code generation for efficiency.
 * Direction numbers from Joe & Kuo (2008).
 */

#include "internal/methods/sobol.h"
#include <string.h>
#include <math.h>

/*============================================================================
 * Direction Numbers (first 40 dimensions)
 *
 * These are the primitive polynomials and initial direction numbers
 * from standard Sobol tables. For production use, load from file.
 *============================================================================*/

/* Primitive polynomials (degree, coefficients) */
static const uint32_t sobol_poly[] = {
    1, 0,    /* dim 1: x */
    2, 1,    /* dim 2: x^2 + x + 1 */
    3, 1,    /* dim 3: x^3 + x + 1 */
    3, 2,    /* dim 4: x^3 + x^2 + 1 */
    4, 1,    /* dim 5: x^4 + x + 1 */
    4, 4,    /* dim 6: x^4 + x^3 + 1 */
    5, 2,    /* dim 7 */
    5, 4,    /* dim 8 */
    5, 7,    /* dim 9 */
    5, 11,   /* dim 10 */
    5, 13,   /* dim 11 */
    5, 14,   /* dim 12 */
    6, 1,    /* dim 13 */
    6, 13,   /* dim 14 */
    6, 16,   /* dim 15 */
    6, 19,   /* dim 16 */
    6, 22,   /* dim 17 */
    6, 25,   /* dim 18 */
    7, 1,    /* dim 19 */
    7, 4,    /* dim 20 */
    7, 7,    /* dim 21 */
    7, 8,    /* dim 22 */
    7, 14,   /* dim 23 */
    7, 19,   /* dim 24 */
    7, 21,   /* dim 25 */
    7, 28,   /* dim 26 */
    7, 31,   /* dim 27 */
    7, 32,   /* dim 28 */
    7, 37,   /* dim 29 */
    7, 41,   /* dim 30 */
    7, 42,   /* dim 31 */
    7, 50,   /* dim 32 */
    7, 55,   /* dim 33 */
    7, 56,   /* dim 34 */
    7, 59,   /* dim 35 */
    7, 62,   /* dim 36 */
    8, 14,   /* dim 37 */
    8, 21,   /* dim 38 */
    8, 22,   /* dim 39 */
    8, 38,   /* dim 40 */
};

/* Initial direction numbers (m values) */
static const uint32_t sobol_m[][10] = {
    {1},                              /* dim 1 */
    {1, 1},                           /* dim 2 */
    {1, 3, 1},                        /* dim 3 */
    {1, 3, 3},                        /* dim 4 */
    {1, 1, 1, 1},                     /* dim 5 */
    {1, 1, 3, 3},                     /* dim 6 */
    {1, 3, 5, 13, 7},                 /* dim 7 */
    {1, 1, 5, 5, 21},                 /* dim 8 */
    {1, 3, 1, 15, 21},                /* dim 9 */
    {1, 3, 7, 5, 27},                 /* dim 10 */
    {1, 1, 5, 11, 19},                /* dim 11 */
    {1, 3, 5, 1, 1},                  /* dim 12 */
    {1, 1, 1, 3, 29, 15},             /* dim 13 */
    {1, 1, 3, 7, 7, 49},              /* dim 14 */
    {1, 1, 1, 9, 19, 21},             /* dim 15 */
    {1, 1, 1, 13, 21, 55},            /* dim 16 */
    {1, 1, 7, 5, 7, 11},              /* dim 17 */
    {1, 1, 7, 7, 31, 17},             /* dim 18 */
    {1, 3, 7, 13, 1, 5, 49},          /* dim 19 */
    {1, 1, 5, 3, 17, 57, 97},         /* dim 20 */
    {1, 1, 7, 1, 7, 33, 73},          /* dim 21 */
    {1, 3, 3, 9, 23, 47, 97},         /* dim 22 */
    {1, 3, 7, 5, 5, 27, 39},          /* dim 23 */
    {1, 3, 1, 3, 21, 3, 7},           /* dim 24 */
    {1, 1, 5, 11, 29, 17, 117},       /* dim 25 */
    {1, 1, 3, 15, 15, 49, 125},       /* dim 26 */
    {1, 3, 1, 11, 19, 7, 3},          /* dim 27 */
    {1, 1, 7, 7, 25, 5, 85},          /* dim 28 */
    {1, 1, 7, 13, 29, 51, 107},       /* dim 29 */
    {1, 3, 5, 13, 31, 55, 89},        /* dim 30 */
    {1, 1, 1, 5, 11, 51, 69},         /* dim 31 */
    {1, 1, 3, 7, 17, 39, 127},        /* dim 32 */
    {1, 1, 1, 9, 1, 33, 83},          /* dim 33 */
    {1, 3, 5, 7, 19, 29, 73},         /* dim 34 */
    {1, 3, 5, 5, 1, 37, 101},         /* dim 35 */
    {1, 3, 3, 11, 29, 33, 93},        /* dim 36 */
    {1, 3, 1, 3, 25, 29, 127, 151},   /* dim 37 */
    {1, 1, 7, 11, 5, 5, 23, 69},      /* dim 38 */
    {1, 3, 3, 1, 31, 51, 95, 243},    /* dim 39 */
    {1, 3, 3, 15, 17, 41, 83, 247},   /* dim 40 */
};

/*============================================================================
 * Initialization
 *============================================================================*/

int mco_sobol_init(mco_sobol *sobol, uint32_t dim)
{
    if (!sobol || dim == 0 || dim > MCO_SOBOL_MAX_DIM) {
        return -1;
    }

    memset(sobol, 0, sizeof(*sobol));
    sobol->dim = dim;
    sobol->count = 0;

    /* Initialize direction numbers for each dimension */
    for (uint32_t d = 0; d < dim; ++d) {
        if (d == 0) {
            /* First dimension: v[k] = 2^(32-k) */
            for (int k = 0; k < MCO_SOBOL_BITS; ++k) {
                sobol->v[0][k] = 1U << (MCO_SOBOL_BITS - 1 - k);
            }
        } else if (d < 40) {
            /* Use tabulated direction numbers */
            uint32_t deg = sobol_poly[d * 2];
            uint32_t poly = sobol_poly[d * 2 + 1];

            /* Initialize first 'deg' direction numbers from table */
            for (uint32_t k = 0; k < deg && k < 10; ++k) {
                sobol->v[d][k] = sobol_m[d][k] << (MCO_SOBOL_BITS - 1 - k);
            }

            /* Generate remaining direction numbers using recurrence */
            for (uint32_t k = deg; k < MCO_SOBOL_BITS; ++k) {
                uint32_t v_k = sobol->v[d][k - deg];
                v_k ^= (sobol->v[d][k - deg] >> deg);

                for (uint32_t j = 1; j < deg; ++j) {
                    if (poly & (1U << (deg - 1 - j))) {
                        v_k ^= sobol->v[d][k - j];
                    }
                }
                sobol->v[d][k] = v_k;
            }
        } else {
            /* For dimensions > 40, use simple fallback (not optimal) */
            for (int k = 0; k < MCO_SOBOL_BITS; ++k) {
                sobol->v[d][k] = ((d * 2654435761U) ^ ((uint32_t)k * 1597334677U))
                                  << (MCO_SOBOL_BITS - 1 - k);
                sobol->v[d][k] |= 1U << (MCO_SOBOL_BITS - 1 - k);
            }
        }
    }

    return 0;
}

/*============================================================================
 * Generation
 *============================================================================*/

/*
 * Find position of rightmost zero bit (for gray code)
 */
static inline int rightmost_zero(uint32_t n)
{
    int c = 0;
    while ((n & 1) == 1) {
        n >>= 1;
        c++;
    }
    return c;
}

void mco_sobol_next(mco_sobol *sobol, double *point)
{
    /* Find position of rightmost zero in count (gray code index) */
    int c = rightmost_zero(sobol->count);

    /* XOR in the appropriate direction number */
    double scale = 1.0 / (double)(1ULL << MCO_SOBOL_BITS);

    for (uint32_t d = 0; d < sobol->dim; ++d) {
        sobol->x[d] ^= sobol->v[d][c];
        point[d] = (double)sobol->x[d] * scale;
    }

    sobol->count++;
}

void mco_sobol_skip(mco_sobol *sobol, uint64_t n)
{
    for (uint64_t i = 0; i < n; ++i) {
        int c = rightmost_zero(sobol->count);
        for (uint32_t d = 0; d < sobol->dim; ++d) {
            sobol->x[d] ^= sobol->v[d][c];
        }
        sobol->count++;
    }
}

void mco_sobol_reset(mco_sobol *sobol)
{
    sobol->count = 0;
    for (uint32_t d = 0; d < sobol->dim; ++d) {
        sobol->x[d] = 0;
    }
}

/*============================================================================
 * Inverse Normal CDF (Moro's Algorithm)
 *============================================================================*/

double mco_sobol_inv_normal(double u)
{
    /* Moro's algorithm for inverse normal CDF */
    static const double a[] = {
        2.50662823884,
        -18.61500062529,
        41.39119773534,
        -25.44106049637
    };
    static const double b[] = {
        -8.47351093090,
        23.08336743743,
        -21.06224101826,
        3.13082909833
    };
    static const double c[] = {
        0.3374754822726147,
        0.9761690190917186,
        0.1607979714918209,
        0.0276438810333863,
        0.0038405729373609,
        0.0003951896511919,
        0.0000321767881768,
        0.0000002888167364,
        0.0000003960315187
    };

    double x = u - 0.5;
    double r;

    if (fabs(x) < 0.42) {
        /* Central region */
        r = x * x;
        r = x * (((a[3] * r + a[2]) * r + a[1]) * r + a[0])
              / ((((b[3] * r + b[2]) * r + b[1]) * r + b[0]) * r + 1.0);
    } else {
        /* Tail region */
        r = (x > 0.0) ? 1.0 - u : u;
        r = log(-log(r));
        r = c[0] + r * (c[1] + r * (c[2] + r * (c[3] + r * (c[4]
            + r * (c[5] + r * (c[6] + r * (c[7] + r * c[8])))))));
        if (x < 0.0) r = -r;
    }

    return r;
}

void mco_sobol_next_normal(mco_sobol *sobol, double *normal)
{
    double uniform[MCO_SOBOL_MAX_DIM];
    mco_sobol_next(sobol, uniform);

    for (uint32_t d = 0; d < sobol->dim; ++d) {
        /* Clamp to avoid infinity at 0 and 1 */
        double u = uniform[d];
        if (u < 1e-10) u = 1e-10;
        if (u > 1.0 - 1e-10) u = 1.0 - 1e-10;
        normal[d] = mco_sobol_inv_normal(u);
    }
}
