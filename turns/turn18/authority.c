#include "authority.h"

#include <math.h>
#include <string.h>

static int valid_mode(ARMode mode) {
    return mode >= AR_SLOW && mode < AR_MODES;
}

static int valid_prices(double cold, double candidate) {
    return isfinite(cold) && isfinite(candidate) &&
           cold <= 1e-9 && candidate <= 1e-9;
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

int ar_quote(const ARState *state, double cold, double candidate, double *live) {
    if (!state || !live || !valid_mode(state->mode) ||
        !valid_prices(cold, candidate) || !isfinite(state->odds) ||
        !isfinite(state->evidence)) return -1;
    double value = !state->active || candidate == cold ? cold :
        logadd(cold, state->odds + candidate) - logadd(0.0, state->odds);
    if (!isfinite(value) || value > 1e-9) return -1;
    *live = value;
    return 0;
}

int ar_observe(ARState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted) {
    if (!state || !slow_used || !admitted || !valid_mode(state->mode) ||
        !valid_prices(cold, candidate) || matched < 0) return -1;
    ARState next = *state;
    double delta = candidate - cold;
    next.shadow += delta;
    if (!isfinite(delta) || !isfinite(next.shadow)) return -1;
    int slow = -1, first = 0;
    if (!state->active) {
        if (next.shadow >= 32.0) {
            next.active = 1;
            next.odds = 0.0; /* source and cold each have mass 1/2 */
            next.evidence = 0.0; /* crossing observation does not enter clock */
            first = 1;
        }
    } else {
        slow = state->mode == AR_SLOW ||
               (state->mode == AR_ADAPTIVE && state->evidence >= 1.0) ||
               (state->mode == AR_WITNESS && state->evidence > -1.0);
        double hazard = slow ? 0x1p-16 : 0x1p-10;
        double posterior = state->odds + delta;
        if (!isfinite(posterior)) return -1;
        next.odds = log1p(-hazard) / log(2.0) -
                    logadd(-posterior, log2(hazard));
        if (state->mode == AR_ADAPTIVE)
            next.evidence = (255.0 / 256.0) * state->evidence + delta;
        /* Silence is not evidence: a byte no stored record voted on leaves w. */
        if (state->mode == AR_WITNESS && matched >= 1)
            next.evidence = (31.0 / 32.0) * state->evidence + delta;
    }
    if (!isfinite(next.odds) || !isfinite(next.evidence)) return -1;
    *state = next;
    *slow_used = slow;
    *admitted = first;
    return 0;
}
