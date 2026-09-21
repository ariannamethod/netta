#include "authority.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* The immutable turn21 API, with names isolated from the new API. Its object
   is compiled from the frozen source with only exported symbols renamed. */
#define AR_SLOW REF_AR_SLOW
#define AR_FAST REF_AR_FAST
#define AR_WITNESS REF_AR_WITNESS
#define AR_GRID REF_AR_GRID
#define AR_MODES REF_AR_MODES
#define ARState RefARState
#define ARMode RefARMode
#define ARCeiling RefARCeiling
#define ar_grid ref_ar_grid
#define ar_init ref_ar_init
#define ar_quote ref_ar_quote
#define ar_observe ref_ar_observe
#include "../turn21/authority.h"
#undef AR_SLOW
#undef AR_FAST
#undef AR_WITNESS
#undef AR_GRID
#undef AR_MODES
#undef ARState
#undef ARMode
#undef ARCeiling
#undef ar_grid
#undef ar_init
#undef ar_quote
#undef ar_observe

static const char *const names[AR_MODES] = {"slow", "fast", "witness", "h8l4", "selfnorm"};

/* One 40-event journey, independent of protocol worlds. Each price pair
   describes one outcome of a binary distribution; its complement is also
   quoted before observation to check the actual prospective mixture. */
int main(void) {
    ARState state[AR_MODES];
    for (int m = 0; m < AR_MODES; m++) assert(ar_init(&state[m], (ARMode)m) == 0);
    RefARState reference[4];
    const RefARMode ref_modes[4] = {REF_AR_SLOW, REF_AR_FAST, REF_AR_WITNESS, REF_AR_GRID + 10};
    assert(ref_ar_grid[10].high == 8 && ref_ar_grid[10].low == 4);
    for (int m = 0; m < 4; m++) assert(ref_ar_init(&reference[m], ref_modes[m]) == 0);
    assert(sizeof(ARState) == 40 && sizeof(RefARState) == 32);
    unsigned continuity_events = 0;
    unsigned clips = 0;
    int have_wrong = 0, have_fast_silence = 0, have_slow_silence = 0;
    double gain[AR_MODES] = {0}, peak[AR_MODES] = {0}, max_norm = 0;
    fputs("{\n  \"scope\": \"one disjoint deterministic synthetic price journey; no protocol worlds\",\n"
          "  \"events\": [\n", stdout);
    for (int t = 0; t < 40; t++) {
        double delta;
        int matched = 1;
        if (t == 0) delta = 32.0;
        else if (t <= 6) delta = 2.0;
        else if (t <= 12) { delta = 0; matched = 0; }
        else if (t == 13) delta = -20.0;
        else if (t <= 17) { delta = 0; matched = 0; }
        else if (t <= 23) delta = 4.0;
        else if (t == 24) delta = -50.0;
        else if (t == 25) { delta = 0; matched = 0; }
        else if (t <= 31) delta = 8.0;
        else if (t <= 35) delta = 0; /* Matched zero decays both statistics. */
        else delta = 4.0;
        double cold = delta > 0 ? -1.0 - delta : -1.0;
        double candidate = cold + delta;
        if (delta == 0) cold = candidate = -8.2806284658485314;
        double other_cold = log1p(-exp2(cold)) / log(2.0);
        double other_candidate = log1p(-exp2(candidate)) / log(2.0);
        ARState before[AR_MODES];
        double live[AR_MODES];
        int slow[AR_MODES], admitted[AR_MODES], clipped[AR_MODES];
        for (int m = 0; m < AR_MODES; m++) {
            before[m] = state[m];
            assert(ar_quote(&state[m], cold, candidate, &live[m]) == 0);
            double other_live;
            assert(ar_quote(&state[m], other_cold, other_candidate, &other_live) == 0);
            assert(memcmp(&state[m], &before[m], sizeof(ARState)) == 0);
            double error = fabs(exp2(live[m]) + exp2(other_live) - 1.0);
            max_norm = fmax(max_norm, error);
            assert(error < 1e-12);
            if (!before[m].active || delta == 0) assert(live[m] == cold);
            if (before[m].active) {
                /* Independent mass-space quote from the BEFORE odds. */
                long double odds = exp2l((long double)before[m].odds);
                long double reference = log2l((exp2l(cold) + odds * exp2l(candidate)) / (1.0L + odds));
                assert(fabsl(reference - live[m]) < 1e-12L);
            }
        }
        double ref_live[4];
        for (int m = 0; m < 4; m++) {
            assert(ref_ar_quote(&reference[m], cold, candidate, &ref_live[m]) == 0);
            assert(ref_live[m] == live[m] && reference[m].odds == before[m].odds &&
                   reference[m].evidence == before[m].evidence && reference[m].shadow == before[m].shadow);
        }
        for (int m = 0; m < AR_MODES; m++) {
            assert(ar_observe(&state[m], cold, candidate, matched,
                              &slow[m], &admitted[m], &clipped[m]) == 0);
            assert(state[m].shadow == state[0].shadow && admitted[m] == admitted[0]);
            gain[m] += live[m] - cold;
            peak[m] = fmax(peak[m], gain[m]);
            double bound = m == AR_FAST ? 10.0 : 16.0;
            assert(gain[m] >= -1.0 - 1e-12 && peak[m] - gain[m] <= bound + 1e-12);
            if (m < AR_H8L4) assert(clipped[m] == 0);
        }
        for (int m = 0; m < 4; m++) {
            int rs, ra, rc;
            assert(ref_ar_observe(&reference[m], cold, candidate, matched, &rs, &ra, &rc) == 0);
            assert(reference[m].odds == state[m].odds && reference[m].evidence == state[m].evidence &&
                   reference[m].shadow == state[m].shadow && reference[m].active == state[m].active &&
                   rs == slow[m] && ra == admitted[m] && rc == clipped[m]);
            assert(state[m].absolute == 0);
            continuity_events++;
        }
        assert(state[AR_SELFNORM].odds <= ar_cap(&state[AR_SELFNORM]) &&
               before[AR_SELFNORM].odds <= ar_cap(&before[AR_SELFNORM]));
        assert(state[AR_SELFNORM].evidence == state[AR_WITNESS].evidence);
        assert(slow[AR_SELFNORM] == slow[AR_WITNESS]);
        assert(state[AR_SELFNORM].absolute >= fabs(state[AR_SELFNORM].evidence) - 1e-10);
        if (t == 0) {
            for (int m = 0; m < AR_MODES; m++)
                assert(admitted[m] == 1 && slow[m] == -1 && state[m].odds == 0 &&
                       state[m].evidence == 0 && state[m].absolute == 0);
        } else {
            assert(admitted[AR_SELFNORM] == 0);
            double expected_a = matched ? (31.0 / 32.0) * before[AR_SELFNORM].absolute + fabs(delta)
                                        : before[AR_SELFNORM].absolute;
            assert(state[AR_SELFNORM].absolute == expected_a);
            /* Measure clipping from this mode's OWN posterior, in mass space. */
            long double prior = exp2l((long double)before[AR_SELFNORM].odds);
            long double posterior = prior * exp2l(delta);
            posterior /= 1.0L + posterior;
            long double hazard = slow[AR_SELFNORM] ? 0x1p-16L : 0x1p-10L;
            long double after_hazard = (1.0L - hazard) * posterior;
            long double unbounded_odds = log2l(after_hazard / (1.0L - after_hazard));
            long double limit = 16.0L * fmaxl(0, state[AR_SELFNORM].evidence) /
                                (1.0L + state[AR_SELFNORM].absolute);
            assert(clipped[AR_SELFNORM] == (unbounded_odds > limit));
            assert(fabsl(state[AR_SELFNORM].odds - fminl(unbounded_odds, limit)) < 1e-10L);
        }
        clips += (unsigned)clipped[AR_SELFNORM];
        if (matched == 0) {
            assert(state[AR_SELFNORM].evidence == before[AR_SELFNORM].evidence);
            assert(state[AR_SELFNORM].absolute == before[AR_SELFNORM].absolute);
            assert(!clipped[AR_SELFNORM] && state[AR_SELFNORM].odds < before[AR_SELFNORM].odds);
            have_fast_silence |= slow[AR_SELFNORM] == 0;
            have_slow_silence |= slow[AR_SELFNORM] == 1;
        }
        if (delta < 0) {
            assert(slow[AR_SELFNORM] == (before[AR_SELFNORM].evidence > -1.0));
            have_wrong = 1;
        }
        printf("%s    {\"t\":%d,\"cold\":%.17g,\"candidate\":%.17g,\"matched\":%d,\"modes\":{",
               t ? ",\n" : "", t, cold, candidate, matched);
        for (int m = 0; m < AR_MODES; m++) {
            printf("%s\"%s\":{\"live\":%.17g,\"shadow_before\":%.17g,\"active_before\":%u,"
                   "\"odds_before\":%.17g,\"evidence_before\":%.17g,\"slow_used\":%d,"
                   "\"admitted\":%d,\"clipped\":%d,\"odds_after\":%.17g,\"evidence_after\":%.17g,"
                   "\"a_before\":%.17g,\"a_after\":%.17g",
                   m ? "," : "", names[m], live[m], before[m].shadow, before[m].active,
                   before[m].odds, before[m].evidence, slow[m], admitted[m], clipped[m],
                   state[m].odds, state[m].evidence, before[m].absolute, state[m].absolute);
            if (m >= AR_H8L4) printf(",\"cap_before\":%.17g,\"cap_after\":%.17g}", ar_cap(&before[m]), ar_cap(&state[m]));
            else fputs(",\"cap_before\":null,\"cap_after\":null}", stdout);
        }
        fputs("}}", stdout);
    }
    assert(clips && have_wrong && have_fast_silence && have_slow_silence);
    printf("\n  ],\n  \"pass\":true,\n  \"clip_count\":%u,\n  \"maximum_binary_normalization_error\":%.17g,\n"
           "  \"sizeof_ARState\":%zu,\n  \"total_prediction_state_bytes\":%zu,\n"
           "  \"exact_turn21_control_events\":%u\n}\n",
           clips, max_norm, sizeof(ARState), sizeof(state), continuity_events);
    return 0;
}
