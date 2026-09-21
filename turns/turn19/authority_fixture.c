#include "authority.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *const names[AR_MODES] = {"slow", "fast", "adaptive", "witness", "ceiling"};

/* One 33-event journey, independent of protocol worlds. Each price pair
   describes one outcome of a binary distribution; its complement is also
   quoted before observation to check the actual prospective mixture. */
int main(void) {
    ARState state[AR_MODES];
    for (int m = 0; m < AR_MODES; m++) assert(ar_init(&state[m], (ARMode)m) == 0);
    assert(sizeof(ARState) == 32);
    unsigned clips = 0;
    int have_wrong = 0, have_fast_silence = 0, have_slow_silence = 0;
    double gain[AR_MODES] = {0}, peak[AR_MODES] = {0}, max_norm = 0;
    fputs("{\n  \"scope\": \"one disjoint deterministic synthetic price journey; no protocol worlds\",\n"
          "  \"events\": [\n", stdout);
    for (int t = 0; t < 33; t++) {
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
        else delta = 0; /* A matched zero-delta event still decays the clock. */
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
        for (int m = 0; m < AR_MODES; m++) {
            assert(ar_observe(&state[m], cold, candidate, matched,
                              &slow[m], &admitted[m], &clipped[m]) == 0);
            assert(state[m].shadow == state[0].shadow && admitted[m] == admitted[0]);
            gain[m] += live[m] - cold;
            peak[m] = fmax(peak[m], gain[m]);
            double bound = m == AR_CEILING ? log2(33.0) : m == AR_FAST ? 10.0 : 16.0;
            assert(gain[m] >= -1.0 - 1e-12 && peak[m] - gain[m] <= bound + 1e-12);
            if (m != AR_CEILING) assert(clipped[m] == 0);
        }
        assert(state[AR_CEILING].odds <= 5 && before[AR_CEILING].odds <= 5);
        assert(state[AR_CEILING].evidence == state[AR_WITNESS].evidence);
        assert(slow[AR_CEILING] == slow[AR_WITNESS]);
        if (t == 0) {
            for (int m = 0; m < AR_MODES; m++)
                assert(admitted[m] == 1 && slow[m] == -1 && state[m].odds == 0 && state[m].evidence == 0);
        } else {
            assert(admitted[AR_CEILING] == 0);
            /* Measure clipping from this mode's OWN posterior, in mass space. */
            long double prior = exp2l((long double)before[AR_CEILING].odds);
            long double posterior = prior * exp2l(delta);
            posterior /= 1.0L + posterior;
            long double hazard = slow[AR_CEILING] ? 0x1p-16L : 0x1p-10L;
            long double after_hazard = (1.0L - hazard) * posterior;
            long double unbounded_odds = log2l(after_hazard / (1.0L - after_hazard));
            assert(clipped[AR_CEILING] == (unbounded_odds > 5.0L));
            assert(fabsl(state[AR_CEILING].odds - fminl(unbounded_odds, 5.0L)) < 1e-10L);
        }
        clips += (unsigned)clipped[AR_CEILING];
        if (matched == 0) {
            assert(state[AR_CEILING].evidence == before[AR_CEILING].evidence);
            assert(!clipped[AR_CEILING] && state[AR_CEILING].odds < before[AR_CEILING].odds);
            have_fast_silence |= slow[AR_CEILING] == 0;
            have_slow_silence |= slow[AR_CEILING] == 1;
        }
        if (delta < 0) {
            assert(slow[AR_CEILING] == (before[AR_CEILING].evidence > -1.0));
            have_wrong = 1;
        }
        printf("%s    {\"t\":%d,\"cold\":%.17g,\"candidate\":%.17g,\"matched\":%d,\"modes\":{",
               t ? ",\n" : "", t, cold, candidate, matched);
        for (int m = 0; m < AR_MODES; m++)
            printf("%s\"%s\":{\"live\":%.17g,\"shadow_before\":%.17g,\"active_before\":%u,"
                   "\"odds_before\":%.17g,\"evidence_before\":%.17g,\"slow_used\":%d,"
                   "\"admitted\":%d,\"clipped\":%d,\"odds_after\":%.17g,\"evidence_after\":%.17g}",
                   m ? "," : "", names[m], live[m], before[m].shadow, before[m].active,
                   before[m].odds, before[m].evidence, slow[m], admitted[m], clipped[m],
                   state[m].odds, state[m].evidence);
        fputs("}}", stdout);
    }
    assert(clips && have_wrong && have_fast_silence && have_slow_silence);
    printf("\n  ],\n  \"pass\":true,\n  \"clip_count\":%u,\n  \"maximum_binary_normalization_error\":%.17g,\n"
           "  \"sizeof_ARState\":%zu,\n  \"total_prediction_state_bytes\":%zu\n}\n",
           clips, max_norm, sizeof(ARState), sizeof(state));
    return 0;
}
