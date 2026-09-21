#include "authority.h"

#include <math.h>
#include <string.h>

static int valid_mode(ARMode mode) {
    return mode >= AR_SLOW && mode < AR_MODES;
}

static int valid_prices(double cold, double candidate) {
    return isfinite(cold) && isfinite(candidate) && cold <= 1e-9 && candidate <= 1e-9;
}

static double logadd(double a, double b) {
    if (a == -INFINITY) return b;
    if (b == -INFINITY) return a;
    double high = a >= b ? a : b;
    double low = a >= b ? b : a;
    return high + log1p(exp2(low - high)) / log(2.0);
}

double ar_initial_odds(ARMode mode) {
    if (!valid_mode(mode)) return NAN;
    if (mode == AR_SLOW || mode == AR_FAST) return 0.0;
    double cold_mass = exp2(-0.5);
    return log2((1.0 - cold_mass) / cold_mass);
}

int ar_init(ARState *state, ARMode mode) {
    if (!state || !valid_mode(mode)) return -1;
    memset(state, 0, sizeof(*state));
    state->mode = mode;
    return 0;
}

int ar_quote(const ARState *state, double cold, double candidate, double *live) {
    if (!state || !live || !valid_mode(state->mode) ||
        !valid_prices(cold, candidate) || !isfinite(state->odds)) return -1;
    /* This is the exact identity lost in the inherited Python fast replay. */
    double value = !state->active || candidate == cold ? cold :
        logadd(cold, state->odds + candidate) - logadd(0.0, state->odds);
    if (!isfinite(value) || value > 1e-9) return -1;
    *live = value;
    return 0;
}

int ar_observe(ARState *state, double cold, double candidate, AREvent *event) {
    if (!state || !event || !valid_mode(state->mode) ||
        !valid_prices(cold, candidate)) return -1;
    ARState next = *state;
    AREvent change = AR_NONE;
    double delta = candidate - cold;
    next.shadow += delta;
    if (!isfinite(delta) || !isfinite(next.shadow)) return -1;
    if (!state->active) {
        if (next.shadow >= 32.0) {
            next.active = 1;
            next.odds = ar_initial_odds(next.mode);
            change = AR_ADMITTED;
        }
        /* The crossing byte cannot arm or pay renewed support. */
    } else {
        double hazard = next.mode == AR_SLOW ? 0x1p-16 : 0x1p-10;
        double posterior = state->odds + delta;
        if (!isfinite(posterior)) return -1;
        next.odds = log1p(-hazard) / log(2.0) - logadd(-posterior, log2(hazard));
        if (next.mode == AR_RETURN && !next.used) {
            double initial = ar_initial_odds(next.mode);
            if (next.armed) {
                next.support = fmax(0.0, next.support + delta);
                /* Natural recovery has priority even if support also crossed. */
                if (next.odds >= initial) {
                    next.armed = 0;
                    next.support = 0.0;
                    change = AR_NATURAL_RECOVERY;
                } else if (next.support >= 32.0) {
                    next.odds = initial;
                    next.used = 1;
                    next.armed = 0;
                    next.support = 0.0;
                    change = AR_RETURNED;
                }
            }
            if (!next.armed && !next.used && next.odds <= -32.0) {
                next.armed = 1;
                next.support = 0.0;
                change = AR_ARMED;
            }
        }
    }
    if (!isfinite(next.odds) || !isfinite(next.support)) return -1;
    *state = next;
    *event = change;
    return 0;
}
