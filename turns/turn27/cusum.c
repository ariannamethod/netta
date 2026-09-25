#include "cusum.h"

#include <math.h>
#include <string.h>

/* The last clause is the one-way law as an invariant: a sum at or above the
   threshold can only be carried by a latched state. */
static int valid_state(const CSState *state) {
    return state && state->core.mode == AR_SLOW &&
        state->core.active <= 1 && state->latched <= 1 &&
        isfinite(state->core.shadow) && isfinite(state->core.odds) &&
        state->core.evidence == 0.0 && state->core.absolute == 0.0 &&
        isfinite(state->sum) && state->sum >= 0.0 &&
        (state->latched || state->sum < CS_THRESHOLD);
}

int cs_init(CSState *state) {
    if (!state) return -1;
    memset(state, 0, sizeof(*state));
    return ar_init(&state->core, AR_SLOW);
}

int cs_quote(const CSState *state, double cold, double candidate, double *live) {
    if (!valid_state(state)) return -1;
    return ar_quote(&state->core, cold, candidate, live);
}

int cs_observe(CSState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted) {
    if (!valid_state(state) || !slow_used || !admitted) return -1;
    CSState next = *state;
    /* Reuse the unchanged odds/admission implementation. The fixed modes run no
       clock of their own, so the sum below is this law's only addition. */
    next.core.mode = state->latched ? AR_FAST : AR_SLOW;
    int slow, first, clipped;
    if (ar_observe(&next.core, cold, candidate, matched, &slow, &first, &clipped) || clipped)
        return -1;
    next.core.mode = AR_SLOW;
    if (state->core.active) {
        if (matched >= 1) {
            double raised = state->sum + (-(candidate - cold) - CS_DRIFT);
            next.sum = raised > 0.0 ? raised : 0.0;
        }
        if (!isfinite(next.sum)) return -1;
        if (next.sum >= CS_THRESHOLD) next.latched = 1;
    } else if (first) {
        next.sum = 0.0;
        next.latched = 0;
    }
    if (!valid_state(&next)) return -1;
    *state = next;
    *slow_used = slow;
    *admitted = first;
    return 0;
}
