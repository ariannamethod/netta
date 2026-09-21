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

int ar_init(ARState *state, ARMode mode) {
    if (!state || !valid_mode(mode)) return -1;
    memset(state, 0, sizeof(*state));
    state->mode = mode;
    return 0;
}

double ar_cap(const ARState *state) {
    if (!state) return NAN;
    if (state->mode == AR_H8L4) return state->evidence <= -1.0 ? 4.0 : 8.0;
    if (state->mode == AR_SELFNORM)
        return 16.0 * fmax(0.0, state->evidence) / (1.0 + state->absolute);
    return NAN;
}

int ar_quote(const ARState *state, double cold, double candidate, double *live) {
    if (!state || !live || !valid_mode(state->mode) ||
        !valid_prices(cold, candidate) || !isfinite(state->odds) ||
        !isfinite(state->evidence) || !isfinite(state->absolute)) return -1;
    double value = !state->active || candidate == cold ? cold :
        logadd(cold, state->odds + candidate) - logadd(0.0, state->odds);
    if (!isfinite(value) || value > 1e-9) return -1;
    *live = value;
    return 0;
}

int ar_observe(ARState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted, int *clipped) {
    if (!state || !slow_used || !admitted || !clipped || !valid_mode(state->mode) ||
        !valid_prices(cold, candidate) || matched < 0) return -1;
    ARState next = *state;
    double delta = candidate - cold;
    next.shadow += delta;
    if (!isfinite(delta) || !isfinite(next.shadow)) return -1;
    int slow = -1, first = 0, cap = 0;
    if (!state->active) {
        if (next.shadow >= 32.0) {
            next.active = 1;
            next.odds = 0.0;
            next.evidence = 0.0;
            next.absolute = 0.0;
            first = 1;
        }
    } else {
        slow = state->mode == AR_SLOW ||
               (state->mode != AR_FAST && state->evidence > -1.0);
        double hazard = slow ? 0x1p-16 : 0x1p-10;
        double posterior = state->odds + delta;
        if (!isfinite(posterior)) return -1;
        next.odds = log1p(-hazard) / log(2.0) - logadd(-posterior, log2(hazard));
        if (state->mode >= AR_WITNESS && matched >= 1)
            next.evidence = (31.0 / 32.0) * state->evidence + delta;
        if (state->mode == AR_SELFNORM && matched >= 1)
            next.absolute = (31.0 / 32.0) * state->absolute + fabs(delta);
        if (state->mode >= AR_H8L4) {
            double limit = ar_cap(&next);
            if (!isfinite(limit) || limit < 0.0 || limit > 16.0) return -1;
            if (next.odds > limit) {
                next.odds = limit;
                cap = 1;
            }
        }
    }
    if (!isfinite(next.odds) || !isfinite(next.evidence) || !isfinite(next.absolute)) return -1;
    if (next.mode == AR_SELFNORM &&
        (next.absolute < 0.0 || fabs(next.evidence) > next.absolute + 1e-10)) return -1;
    *state = next;
    *slow_used = slow;
    *admitted = first;
    *clipped = cap;
    return 0;
}
