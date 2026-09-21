#include "authority.h"

#include <math.h>
#include <string.h>

/* The preregistered grid: high in {5,6,8,10,12,16}, low in {2,3,4,5}, low <= high.
   Declared here as constants, exactly as turn20 declared its (10, 5). */
const ARCeiling ar_grid[AR_GRID_POINTS] = {
    { 5, 2}, { 5, 3}, { 5, 4}, { 5, 5},
    { 6, 2}, { 6, 3}, { 6, 4}, { 6, 5},
    { 8, 2}, { 8, 3}, { 8, 4}, { 8, 5},
    {10, 2}, {10, 3}, {10, 4}, {10, 5},
    {12, 2}, {12, 3}, {12, 4}, {12, 5},
    {16, 2}, {16, 3}, {16, 4}, {16, 5},
};

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
            next.odds = 0.0; /* source and cold each have mass 1/2 */
            next.evidence = 0.0; /* crossing observation does not enter clock */
            first = 1;
        }
    } else {
        slow = state->mode == AR_SLOW ||
               (state->mode != AR_FAST && state->evidence > -1.0);
        double hazard = slow ? 0x1p-16 : 0x1p-10;
        double posterior = state->odds + delta;
        if (!isfinite(posterior)) return -1;
        next.odds = log1p(-hazard) / log(2.0) -
                    logadd(-posterior, log2(hazard));
        /* Silence is not evidence: a byte no stored record voted on leaves w. */
        if (state->mode >= AR_WITNESS && matched >= 1)
            next.evidence = (31.0 / 32.0) * state->evidence + delta;
        /* Charge used the old quote. Only this state's own next odds are capped,
           at this grid point's low level when the updated witness evidence says
           the memory is currently wrong, otherwise at its high level. */
        if (state->mode >= AR_GRID) {
            const ARCeiling *level = &ar_grid[state->mode - AR_GRID];
            double limit = next.evidence <= -1.0 ? (double)level->low
                                                 : (double)level->high;
            if (next.odds > limit) {
                next.odds = limit;
                cap = 1;
            }
        }
    }
    if (!isfinite(next.odds) || !isfinite(next.evidence)) return -1;
    *state = next;
    *slow_used = slow;
    *admitted = first;
    *clipped = cap;
    return 0;
}
