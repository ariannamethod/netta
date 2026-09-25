#include "latch.h"

#include <math.h>
#include <string.h>

static int valid_state(const LRState *state) {
    return state && state->core.mode == AR_WITNESS &&
        state->core.active <= 1 && state->fast_latched <= 1 &&
        isfinite(state->core.shadow) && isfinite(state->core.odds) &&
        isfinite(state->core.evidence) && state->core.absolute == 0.0;
}

int lr_init(LRState *state) {
    if (!state) return -1;
    memset(state, 0, sizeof(*state));
    return ar_init(&state->core, AR_WITNESS);
}

int lr_quote(const LRState *state, double cold, double candidate, double *live) {
    if (!valid_state(state)) return -1;
    return ar_quote(&state->core, cold, candidate, live);
}

int lr_observe(LRState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted) {
    if (!valid_state(state) || !slow_used || !admitted) return -1;
    LRState next = *state;
    /* Reuse the unchanged odds/admission implementation. Fixed modes do not
       run its witness clock; restore that clock exactly once below. */
    next.core.mode = state->fast_latched ? AR_FAST : AR_SLOW;
    int slow, first, clipped;
    if (ar_observe(&next.core, cold, candidate, matched, &slow, &first, &clipped) || clipped)
        return -1;
    next.core.mode = AR_WITNESS;
    if (state->core.active) {
        if (matched >= 1)
            next.core.evidence = (31.0 / 32.0) * state->core.evidence + (candidate - cold);
        if (!isfinite(next.core.evidence)) return -1;
        if (next.core.evidence <= -1.0) next.fast_latched = 1;
        else if (next.core.evidence >= 1.0) next.fast_latched = 0;
    } else if (first) {
        next.fast_latched = 0;
    }
    if (!valid_state(&next)) return -1;
    *state = next;
    *slow_used = slow;
    *admitted = first;
    return 0;
}
