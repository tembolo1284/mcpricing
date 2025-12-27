/*
 * Option payoff functions
 *
 * These are the terminal payoffs for various option types.
 * All functions are pure - no side effects, no state.
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_PAYOFF_H
#define MCO_INTERNAL_INSTRUMENTS_PAYOFF_H

/*
 * Option type enumeration (internal use)
 */
#ifndef MCO_OPTION_TYPE_DEFINED
#define MCO_OPTION_TYPE_DEFINED
typedef enum {
    MCO_CALL = 0,
    MCO_PUT  = 1
} mco_option_type;
#endif

/*
 * Vanilla payoffs
 */
static inline double mco_payoff_call(double spot, double strike)
{
    return (spot > strike) ? (spot - strike) : 0.0;
}

static inline double mco_payoff_put(double spot, double strike)
{
    return (strike > spot) ? (strike - spot) : 0.0;
}

static inline double mco_payoff(double spot, double strike, mco_option_type type)
{
    return (type == MCO_CALL) ? mco_payoff_call(spot, strike) 
                              : mco_payoff_put(spot, strike);
}

/*
 * Asian option payoffs (arithmetic average)
 */
static inline double mco_payoff_asian_call(double avg_spot, double strike)
{
    return (avg_spot > strike) ? (avg_spot - strike) : 0.0;
}

static inline double mco_payoff_asian_put(double avg_spot, double strike)
{
    return (strike > avg_spot) ? (strike - avg_spot) : 0.0;
}

/*
 * Lookback payoffs (floating strike)
 */
static inline double mco_payoff_lookback_call(double spot, double min_spot)
{
    return spot - min_spot;  /* Always >= 0 by definition */
}

static inline double mco_payoff_lookback_put(double max_spot, double spot)
{
    return max_spot - spot;  /* Always >= 0 by definition */
}

#endif /* MCO_INTERNAL_INSTRUMENTS_PAYOFF_H */
