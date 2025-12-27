/*
 * Digital (Binary) Options
 *
 * Digital options pay a fixed amount if they finish in-the-money,
 * otherwise they pay nothing.
 *
 * Types:
 *   Cash-or-Nothing:
 *     - Call: pays Q if S(T) > K
 *     - Put:  pays Q if S(T) < K
 *
 *   Asset-or-Nothing:
 *     - Call: pays S(T) if S(T) > K
 *     - Put:  pays S(T) if S(T) < K
 *
 * Properties:
 *   - Discontinuous payoff at strike
 *   - High gamma/vega near strike at expiry
 *   - Analytical formulas (simple Black-Scholes variants)
 *   - Often used in structured products
 *
 * Pricing:
 *   Cash-or-Nothing Call = Q · e^(-rT) · N(d₂)
 *   Asset-or-Nothing Call = S · N(d₁)
 */

#ifndef MCO_INTERNAL_INSTRUMENTS_DIGITAL_H
#define MCO_INTERNAL_INSTRUMENTS_DIGITAL_H

#include "internal/context.h"
#include "internal/instruments/payoff.h"
#include <stddef.h>

/*
 * Digital option type
 */
typedef enum {
    MCO_DIGITAL_CASH  = 0,  /* Cash-or-nothing */
    MCO_DIGITAL_ASSET = 1   /* Asset-or-nothing */
} mco_digital_type;

/*
 * Price a digital option using Monte Carlo
 */
double mco_price_digital(mco_ctx *ctx,
                         double spot,
                         double strike,
                         double payout,      /* Q for cash-or-nothing */
                         double rate,
                         double volatility,
                         double time,
                         mco_digital_type digital_type,
                         mco_option_type option_type);

/*
 * Analytical digital option formulas
 */

/* Cash-or-nothing */
double mco_digital_cash_call(double spot, double strike, double payout,
                              double rate, double vol, double time);
double mco_digital_cash_put(double spot, double strike, double payout,
                             double rate, double vol, double time);

/* Asset-or-nothing */
double mco_digital_asset_call(double spot, double strike,
                               double rate, double vol, double time);
double mco_digital_asset_put(double spot, double strike,
                              double rate, double vol, double time);

#endif /* MCO_INTERNAL_INSTRUMENTS_DIGITAL_H */
