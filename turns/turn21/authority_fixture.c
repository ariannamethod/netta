#include "authority.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static char names[AR_MODES][16];

static void name_modes(void) {
    snprintf(names[AR_SLOW], sizeof(names[0]), "slow");
    snprintf(names[AR_FAST], sizeof(names[0]), "fast");
    snprintf(names[AR_WITNESS], sizeof(names[0]), "witness");
    for (int i = 0; i < AR_GRID_POINTS; i++)
        snprintf(names[AR_GRID + i], sizeof(names[0]), "h%dl%d",
                 ar_grid[i].high, ar_grid[i].low);
}

/* One 33-event journey, independent of protocol worlds. Each price pair
   describes one outcome of a binary distribution; its complement is also
   quoted before observation to check the actual prospective mixture. */
int main(void) {
    ARState state[AR_MODES];
    name_modes();
    for (int m = 0; m < AR_MODES; m++) assert(ar_init(&state[m], (ARMode)m) == 0);
    assert(sizeof(ARState) == 32);
    assert(AR_MODES == AR_GRID + AR_GRID_POINTS && AR_GRID_POINTS == 24);
    unsigned low_clips[AR_GRID_POINTS] = {0}, high_clips[AR_GRID_POINTS] = {0};
    unsigned total_low = 0, total_high = 0;
    int have_wrong = 0, have_fast_silence = 0, have_slow_silence = 0;
    double gain[AR_MODES] = {0}, peak[AR_MODES] = {0}, max_norm = 0;
    /* The preregistered grid, rebuilt here independently of authority.c. */
    const int highs[6] = {5, 6, 8, 10, 12, 16}, lows[4] = {2, 3, 4, 5};
    for (int i = 0; i < AR_GRID_POINTS; i++) {
        assert(ar_grid[i].high == highs[i / 4] && ar_grid[i].low == lows[i % 4]);
        assert(ar_grid[i].low <= ar_grid[i].high);
    }
    fputs("{\n  \"scope\": \"one disjoint deterministic synthetic price journey; no protocol worlds\",\n"
          "  \"events\": [\n", stdout);
    for (int t = 0; t < 33; t++) {
        double delta;
        int matched = 1;
        if (t == 0) delta = 32.0;
        else if (t <= 6) delta = 2.0;
        else if (t <= 12) { delta = 0; matched = 0; }
        else if (t == 13) delta = -20.0;
        else if (t <= 17) { delta = 8; matched = 0; }
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
            double bound = m == AR_FAST ? 10.0 : m < AR_GRID ? 16.0 :
                           log2(exp2((double)ar_grid[m - AR_GRID].high) + 1.0);
            assert(gain[m] >= -1.0 - 1e-12 && peak[m] - gain[m] <= bound + 1e-12);
            if (m < AR_GRID) assert(clipped[m] == 0);
        }
        for (int i = 0; i < AR_GRID_POINTS; i++) {
            int m = AR_GRID + i;
            assert(state[m].evidence == state[AR_WITNESS].evidence);
            assert(before[m].evidence == before[AR_WITNESS].evidence);
            assert(slow[m] == slow[AR_WITNESS]);
            double limit = state[m].evidence <= -1.0 ? (double)ar_grid[i].low
                                                     : (double)ar_grid[i].high;
            assert(before[m].odds <= (double)ar_grid[i].high && state[m].odds <= limit);
            if (t == 0) {
                assert(admitted[m] == 1 && slow[m] == -1 &&
                       state[m].odds == 0 && state[m].evidence == 0);
            } else {
                /* Measure clipping from this mode's OWN posterior, in mass space. */
                long double prior = exp2l((long double)before[m].odds);
                long double posterior = prior * exp2l(delta);
                posterior /= 1.0L + posterior;
                long double hazard = slow[m] ? 0x1p-16L : 0x1p-10L;
                long double after_hazard = (1.0L - hazard) * posterior;
                long double unbounded = log2l(after_hazard / (1.0L - after_hazard));
                assert(clipped[m] == (unbounded > (long double)limit));
                assert(fabsl(state[m].odds - fminl(unbounded, (long double)limit)) < 1e-10L);
            }
            if (clipped[m]) {
                if (limit == (double)ar_grid[i].low) { low_clips[i]++; total_low++; }
                else { high_clips[i]++; total_high++; }
            }
        }
        if (t == 0)
            for (int m = 0; m < AR_MODES; m++)
                assert(admitted[m] == 1 && slow[m] == -1 &&
                       state[m].odds == 0 && state[m].evidence == 0);
        if (matched == 0) {
            assert(state[AR_WITNESS].evidence == before[AR_WITNESS].evidence);
            have_fast_silence |= slow[AR_WITNESS] == 0;
            have_slow_silence |= slow[AR_WITNESS] == 1;
        }
        if (delta < 0) {
            assert(slow[AR_WITNESS] == (before[AR_WITNESS].evidence > -1.0));
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
    /* The tightest point must clip at both levels; the loosest high with the
       lowest low must still reach its low level on wrong evidence. */
    assert(total_low && total_high && have_wrong &&
           have_fast_silence && have_slow_silence);
    assert(low_clips[0] && high_clips[0]);
    printf("\n  ],\n  \"pass\":true,\n  \"grid_points\":%d,\n  \"modes\":%d,\n"
           "  \"maximum_binary_normalization_error\":%.17g,\n"
           "  \"total_low_clips\":%u,\n  \"total_high_clips\":%u,\n"
           "  \"per_point_clips\": [",
           AR_GRID_POINTS, (int)AR_MODES, max_norm, total_low, total_high);
    for (int i = 0; i < AR_GRID_POINTS; i++)
        printf("%s\n    {\"point\":\"%s\",\"high\":%d,\"low\":%d,\"low_clips\":%u,\"high_clips\":%u}",
               i ? "," : "", names[AR_GRID + i], ar_grid[i].high, ar_grid[i].low,
               low_clips[i], high_clips[i]);
    printf("\n  ],\n  \"sizeof_ARState\":%zu,\n  \"total_prediction_state_bytes\":%zu\n}\n",
           sizeof(ARState), sizeof(state));
    return 0;
}
